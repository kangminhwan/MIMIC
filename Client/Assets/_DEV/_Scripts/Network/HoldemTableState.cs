using System;
using System.Collections.Generic;
using System.Linq;
using General;
using Mimic.Protocol;
using PmNet;

namespace Mimic.Network
{
    // Converts native casino notifications into the existing Unity screen model.
    // Envelope is a local view model here; it is never sent to TableServer.
    public sealed class HoldemTableState
    {
        public TableSnapshot Snapshot { get; private set; }
        public ulong MemberId { get; private set; }
        public ulong CaptainId { get; private set; }
        public ulong LeadId { get; private set; }
        public AssetKind Asset => asset;
        public IReadOnlyList<TableAction> Actions => actions;
        public bool CanStart => Snapshot != null && LeadId == MemberId &&
            Snapshot.Players.Count >= 2 && Snapshot.Street == Street.Waiting;
        public event Action<TableSnapshot> Changed;
        public event Action Left;
        private readonly List<TableAction> actions = new List<TableAction>();
        private int phase;
        private AssetKind asset;
        private bool roundPrepared;

        public void Receive(PacketID id, PktBase packet)
        {
            if (packet.ErrKind != ResultCode.ResultSuccess) return;
            switch (id)
            {
                case PacketID.PacketAccessOpen:
                    MemberId = SigninRS.Parser.ParseFrom(packet.Payload).MemberInfo?.MemberId ?? 0;
                    return;
                case PacketID.PacketSpaceCreate:
                    var created = ChamberBuildRS.Parser.ParseFrom(packet.Payload);
                    Enter(created.ChamberInfo, created.Members, created.CaptainIdx, created.LeadIdx);
                    break;
                case PacketID.PacketSpaceEnter:
                    var entered = ChamberEnterRS.Parser.ParseFrom(packet.Payload);
                    Enter(entered.ChamberInfo, entered.Members, entered.CaptainIdx, entered.LeadIdx);
                    break;
                case PacketID.PacketSpaceLeave:
                    var leave = ChamberLeaveRS.Parser.ParseFrom(packet.Payload);
                    if (leave.MemberIdx == MemberId) { Snapshot = null; actions.Clear(); Left?.Invoke(); return; }
                    var leaving = Player(leave.MemberIdx);
                    if (leaving != null) Snapshot.Players.Remove(leaving);
                    break;
                case PacketID.PacketTableOwnerNotice:
                    CaptainId = MatchCaptainSwapRS.Parser.ParseFrom(packet.Payload).CaptainIdx;
                    break;
                case PacketID.PacketLeadSeatNotice:
                    var lead = MatchLeadSwapRS.Parser.ParseFrom(packet.Payload);
                    LeadId = lead.LeadIdx;
                    if (Snapshot != null) Snapshot.DealerSeat = Player(lead.LeadIdx) is PlayerState dealer ? (int)dealer.Seat : -1;
                    break;
                case PacketID.PacketRoundStateNotice:
                    var status = MatchStateSwapRS.Parser.ParseFrom(packet.Payload);
                    if (status.LeadIdx != 0) LeadId = status.LeadIdx;
                    SetPhase(status.MatchPhase);
                    break;
                case PacketID.PacketTurnNotice:
                    if (Snapshot == null) return;
                    var turn = PhaseTurnRS.Parser.ParseFrom(packet.Payload);
                    SetPhase(turn.MatchPhase);
                    Snapshot.ActingSeat = Player(turn.MemberIdx) is PlayerState active ? (int)active.Seat : -1;
                    Snapshot.CurrentBet = Amount(turn.MaxWager);
                    actions.Clear();
                    if (turn.MemberIdx == MemberId) actions.AddRange(turn.WagerOptions);
                    break;
                case PacketID.PacketInitialDeal:
                    if (Snapshot == null) return;
                    BeginHand();
                    var deal = SeedDealoutRS.Parser.ParseFrom(packet.Payload);
                    foreach (var pair in deal.MemberCards)
                    {
                        SetCards(pair.Key, pair.Value.PlayingCards);
                        var dealtPlayer = Player(pair.Key);
                        if (dealtPlayer == null) continue;
                        long ante = Amount(deal.SeedFunds);
                        dealtPlayer.Chips = checked(dealtPlayer.Chips - ante);
                        dealtPlayer.Committed += ante;
                        Snapshot.Pot += ante;
                    }
                    foreach (var blind in deal.BlindWagers) Wager(blind);
                    break;
                case PacketID.PacketStakeSubmit:
                    Wager(MatchWagerRS.Parser.ParseFrom(packet.Payload));
                    break;
                case PacketID.PacketFinalReveal:
                case PacketID.PacketBoardCardReveal:
                    if (Snapshot == null) return;
                    var reveal = RevealRS.Parser.ParseFrom(packet.Payload);
                    Snapshot.Board.Clear(); Snapshot.Board.Add(reveal.SharedCards.Select(Card));
                    foreach (var pair in reveal.MemberHandTiles) SetCards(pair.Key, pair.Value.PlayingCards);
                    break;
                case PacketID.PacketRoundOutcome:
                    if (Snapshot == null) return;
                    var outcome = MatchOutcomeRS.Parser.ParseFrom(packet.Payload);
                    foreach (var member in outcome.TopSet.Concat(outcome.BottomSet))
                    {
                        var player = Player(member.MemberIdx);
                        if (player == null) continue;
                        player.Chips = Amount(member.FundAfter);
                        SetCards(member.MemberIdx, member.Cards);
                    }
                    Snapshot.Street = Street.Complete; Snapshot.ActingSeat = -1; actions.Clear();
                    Snapshot.Result = string.Join(", ", outcome.TopSet.Select(p => p.AliasLabel + " " + p.HandRank));
                    break;
                default: return;
            }
            Publish();
        }

        private void Enter(RoomSnapshot room, IEnumerable<ParticipantProfile> members, ulong captain, ulong lead)
        {
            if (room == null || room.PlayCategory != PlayCategory.TexasHoldem)
                throw new InvalidOperationException("Expected a Holdem room snapshot.");
            bool sameRoom = Snapshot != null && Snapshot.TableId == (uint)room.RoomNo;
            if (!sameRoom) { Snapshot = new TableSnapshot { ActingSeat = -1, DealerSeat = -1 }; actions.Clear(); roundPrepared = room.PlayPhase >= 2; }
            Snapshot.TableId = checked((uint)room.RoomNo);
            Snapshot.HandId = room.PlayedRounds;
            Snapshot.Pot = Amount(room.PotAmount);
            Snapshot.CurrentBet = Amount(room.PreviousWager);
            asset = room.AssetKind; CaptainId = captain; LeadId = lead;
            Snapshot.Players.Clear();
            uint seat = 0;
            foreach (var member in members)
            {
                if (member.MemberId != 0)
                {
                    var player = new PlayerState {
                        PlayerId = member.MemberId.ToString(), DisplayName = member.DisplayName, Seat = seat,
                        Chips = Amount(asset == AssetKind.Coin ? member.WalletCoins : member.WalletChips),
                        StreetBet = Amount(member.CurrentWagerAmount), Committed = Amount(member.RoundWagerTotal),
                        Folded = member.FoldedOut, Connected = true
                    };
                    player.HoleCards.Add(member.HeldCards.Where(c => (int)c.RankCode != 0).Select(Card));
                    Snapshot.Players.Add(player);
                }
                seat++;
            }
            phase = room.PlayPhase; Snapshot.Street = StreetFor(phase);
            Snapshot.DealerSeat = Player(lead) is PlayerState dealer ? (int)dealer.Seat : -1;
        }

        private void SetPhase(int next)
        {
            if (Snapshot == null) { phase = next; return; }
            if (next == phase) return;
            if (next < 2) roundPrepared = false;
            if (next == 2) BeginHand();
            if (next == 0 || next == 4 || next == 6 || next == 8)
            {
                foreach (var player in Snapshot.Players) player.StreetBet = 0;
                Snapshot.CurrentBet = 0;
            }
            phase = next; Snapshot.Street = StreetFor(next); Snapshot.ActingSeat = -1; actions.Clear();
        }

        private void BeginHand()
        {
            if (roundPrepared) return;
            roundPrepared = true;
            Snapshot.Street = Street.Preflop;
            Snapshot.HandId++; Snapshot.Board.Clear(); Snapshot.Result = ""; Snapshot.Pot = 0; Snapshot.CurrentBet = 0;
            foreach (var player in Snapshot.Players)
            {
                player.HoleCards.Clear(); player.Folded = false; player.Committed = 0; player.StreetBet = 0;
            }
        }

        private void Wager(MatchWagerRS wager)
        {
            var player = Player(wager.WagerMemberIdx);
            if (player == null) return;
            long paid = Amount(wager.FundBefore >= wager.FundAfter ? wager.FundBefore - wager.FundAfter : 0);
            player.Chips = Amount(wager.FundAfter); player.StreetBet += paid; player.Committed += paid;
            player.Folded = wager.MemberWager == TableAction.GiveUp;
            Snapshot.Pot += paid; Snapshot.CurrentBet = Math.Max(Snapshot.CurrentBet, player.StreetBet);
            if (wager.WagerMemberIdx == MemberId) actions.Clear();
        }

        private void SetCards(ulong member, IEnumerable<PlayingCard> cards)
        {
            var player = Player(member); if (player == null) return;
            player.HoleCards.Clear(); player.HoleCards.Add(cards.Where(c => (int)c.RankCode != 0).Select(Card));
        }
        private PlayerState Player(ulong id) => Snapshot?.Players.FirstOrDefault(p => p.PlayerId == id.ToString());
        private void Publish() { if (Snapshot != null) { Snapshot.Revision++; Changed?.Invoke(Snapshot.Clone()); } }
        private static Street StreetFor(int value) => value < 2 ? Street.Waiting : value < 4 ? Street.Preflop :
            value < 6 ? Street.Flop : value < 8 ? Street.Turn : value < 10 ? Street.River : Street.Complete;
        private static long Amount(ulong value) => checked((long)value);
        private static Card Card(PlayingCard card) => new Card {
            Rank = (uint)((int)card.RankCode == 1 ? 14 : (int)card.RankCode),
            Suit = card.SuitCode == CardSuit.Club ? 0u : card.SuitCode == CardSuit.Diamond ? 1u : card.SuitCode == CardSuit.Heart ? 2u : 3u
        };
    }
}
