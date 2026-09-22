#include "Holdem/Table.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <set>
#include <windows.h>
#include <bcrypt.h>

namespace mimic {
namespace {
uint64_t score5(const std::vector<Card>& c) {
    std::array<int, 15> count{};
    bool flush = true;
    for (auto card : c) { ++count[card.rank]; flush &= card.suit == c.front().suit; }
    int straight = 0;
    for (int top = 14; top >= 5; --top) {
        bool found = true;
        for (int k = 0; k < 5; ++k) if (!count[top-k]) found = false;
        if (found) { straight = top; break; }
    }
    if (!straight && count[14] && count[2] && count[3] && count[4] && count[5]) straight = 5;
    std::vector<std::pair<int,int>> groups;
    for (int rank = 14; rank >= 2; --rank) if (count[rank]) groups.emplace_back(count[rank], rank);
    std::sort(groups.rbegin(), groups.rend());
    std::vector<int> kick;
    int category = 0;
    if (flush && straight) { category = 8; kick = {straight}; }
    else if (groups[0].first == 4) { category = 7; kick = {groups[0].second, groups[1].second}; }
    else if (groups[0].first == 3 && groups[1].first == 2) { category = 6; kick = {groups[0].second, groups[1].second}; }
    else if (flush) { category = 5; for (int n = 14; n >= 2; --n) for (int j=0;j<count[n];++j) kick.push_back(n); }
    else if (straight) { category = 4; kick = {straight}; }
    else {
        category = groups[0].first == 3 ? 3 : groups[0].first == 2 ? (groups[1].first == 2 ? 2 : 1) : 0;
        for (auto [number, rank] : groups) kick.push_back(rank);
    }
    uint64_t score = category;
    for (int i=0;i<5;++i) score = score * 15 + (i < static_cast<int>(kick.size()) ? kick[i] : 0);
    return score;
}
uint32_t randomBelow(uint32_t limit) {
    uint32_t n;
    const uint32_t threshold = (uint32_t{0} - limit) % limit;
    do {
        if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&n), sizeof(n), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
            throw std::runtime_error("Random generator failed");
    } while (n < threshold);
    return n % limit;
}
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
}
uint64_t evaluate(const std::vector<Card>& c) {
    require(c.size() >= 5 && c.size() <= 7, "Expected 5 to 7 cards");
    std::set<std::pair<int,int>> unique;
    for (auto card : c) require(card.rank >= 2 && card.rank <= 14 && card.suit >= 0 && card.suit < 4 && unique.emplace(card.rank,card.suit).second, "Invalid or duplicate card");
    uint64_t best = 0;
    for (size_t a=0;a<c.size();++a) for (size_t b=a+1;b<c.size();++b)
    for (size_t d=b+1;d<c.size();++d) for (size_t e=d+1;e<c.size();++e)
    for (size_t f=e+1;f<c.size();++f) best = std::max(best, score5({c[a],c[b],c[d],c[e],c[f]}));
    return best;
}
bool Table::active() const { return street_ != protocol::WAITING && street_ != protocol::COMPLETE; }
int Table::count() const { int n=0; for (auto& p:players_) n += !p.id.empty(); return n; }
int Table::seat(const std::string& id) const { for (int i=0;i<Capacity;++i) if (players_[i].id==id && !id.empty()) return i; throw std::runtime_error("Not seated"); }
bool Table::contains(const std::string& id) const { for(auto& p:players_) if(!id.empty() && p.id==id) return true; return false; }
int Table::next(int from, bool pendingOnly) const {
    for(int d=1;d<=Capacity;++d) { int i=(from+d+Capacity)%Capacity; const auto& p=players_[i];
        if(!p.id.empty() && (!pendingOnly || (!p.folded && p.chips>0 && p.pending))) return i; }
    return -1;
}
void Table::join(const std::string& id,const std::string& name) {
    require(!id.empty(),"Missing player identity");
    require(!active(),"Wait until this hand ends");
    require(!contains(id),"Already seated");
    for(auto& p:players_) if(p.id.empty()) { p=Player{}; p.id=id; p.name=name; ++revision_; return; }
    throw std::runtime_error("Table full");
}
void Table::leave(const std::string& id) {
    require(!active(),"Cannot leave during a hand; disconnect folds your hand");
    players_[seat(id)]=Player{}; ++revision_;
}
void Table::disconnect(const std::string& id) {
    if(!contains(id)) return;
    int i=seat(id);
    if(!active()) { players_[i]=Player{}; ++revision_; return; }
    players_[i].connected=false; players_[i].ready=false;
    // All-in hands stay eligible for showdown after transport loss.
    if(players_[i].chips>0) { players_[i].folded=true; players_[i].pending=false; }
    ++revision_;
    int alive=0; for(auto& p:players_) if(!p.id.empty() && !p.folded) ++alive;
    if(alive<=1 || actor_==i) progress(i);
}
void Table::ready(const std::string& id,bool value) {
    require(!active(),"Hand already running");
    auto& p=players_[seat(id)]; require(p.connected,"Disconnected player");
    require(!value || p.chips>=20,"At least one big blind is required");
    p.ready=value; ++revision_;
    for(auto& item:players_) if(!item.id.empty() && !item.connected) item=Player{};
    if(count()<2) return;
    for(auto& item:players_) if(!item.id.empty() && !item.ready) return;
    start();
}
Card Table::draw() { auto c=deck_.back(); deck_.pop_back(); return c; }
void Table::pay(int i,int64_t amount) {
    auto& p=players_[i]; amount=std::min(amount,p.chips);
    p.chips-=amount; p.bet+=amount; p.committed+=amount;
}
void Table::start() {
    deck_.clear(); board_.clear(); result_.clear();
    for(int suit=0;suit<4;++suit) for(int rank=2;rank<=14;++rank) deck_.push_back({rank,suit});
    for(uint32_t i=51;i>0;--i) std::swap(deck_[i],deck_[randomBelow(i+1)]);
    ++hand_; street_=protocol::PREFLOP; currentBet_=20; minRaise_=20;
    dealer_=next(dealer_);
    for(auto& p:players_) if(!p.id.empty()) {
        p.bet=0;p.committed=0;p.folded=false;p.ready=false;p.pending=true;p.hole={draw(),draw()};
    }
    int sb=count()==2 ? dealer_ : next(dealer_); int bb=next(sb);
    pay(sb,10); pay(bb,20);
    for(auto& p:players_) if(p.chips==0) p.pending=false;
    actor_=next(bb,true);
    progress(bb);
}
void Table::act(const std::string& id,const protocol::ActionRequest& request) {
    require(active(),"No active hand");
    require(request.hand_id()==hand_ && request.revision()==revision_,"Stale hand or revision");
    int i=seat(id); require(i==actor_,"Not your turn");
    auto& p=players_[i]; require(p.connected && !p.folded && p.chips>0,"Cannot act");
    auto owed=currentBet_-p.bet;
    switch(request.kind()) {
    case protocol::FOLD: p.folded=true; break;
    case protocol::CHECK: require(owed==0,"Must call or fold"); break;
    case protocol::CALL: require(owed>0,"Nothing to call"); pay(i,owed); break;
    case protocol::RAISE: {
        const auto to=request.raise_to();
        require(to>currentBet_ && to<=p.bet+p.chips,"Raise outside available stack");
        require(to-currentBet_>=minRaise_,"Minimum full raise required; short all-in raises are not supported by this starter");
        int opponentsWithChips=0; for(int j=0;j<Capacity;++j) if(j!=i && !players_[j].id.empty() && !players_[j].folded && players_[j].chips>0) ++opponentsWithChips;
        require(opponentsWithChips>0,"No opponent can respond to a raise");
        minRaise_=to-currentBet_; currentBet_=to; pay(i,to-p.bet);
        for(auto& other:players_) other.pending=!other.id.empty() && !other.folded && other.chips>0;
        break;
    }
    default: throw std::runtime_error("Unsupported action");
    }
    p.pending=false; ++revision_; progress(i);
}
void Table::progress(int previous) {
    while(active()) {
        int alive=0, movable=0, lone=-1;
        for(int i=0;i<Capacity;++i) { auto& p=players_[i]; if(!p.id.empty() && !p.folded) { ++alive; if(p.chips>0) { ++movable;lone=i; } } }
        if(alive<=1) { finish(); return; }
        if(movable==1 && players_[lone].bet>=currentBet_) players_[lone].pending=false;
        int nextActor=next(previous,true);
        if(nextActor>=0) { actor_=nextActor; return; }
        if(street_==protocol::RIVER) { finish(); return; }
        draw(); // Burn before each community-card street.
        if(street_==protocol::PREFLOP) { board_.push_back(draw());board_.push_back(draw());board_.push_back(draw());street_=protocol::FLOP; }
        else { board_.push_back(draw());street_=street_==protocol::FLOP ? protocol::TURN : protocol::RIVER; }
        currentBet_=0;minRaise_=20;
        for(auto& p:players_) { p.bet=0;p.pending=!p.id.empty() && !p.folded && p.chips>0 && movable>1; }
        previous=dealer_;
    }
}
void Table::finish() {
    std::set<int64_t> levels; for(auto& p:players_) if(p.committed>0) levels.insert(p.committed);
    int64_t previous=0; std::array<int64_t,Capacity> payouts{};
    for(auto level:levels) {
        int contributors=0; std::vector<int> eligible;
        for(int i=0;i<Capacity;++i) if(!players_[i].id.empty() && players_[i].committed>=level) {
            ++contributors; if(!players_[i].folded) eligible.push_back(i);
        }
        int64_t amount=(level-previous)*contributors;previous=level;
        if(eligible.empty()) {
            // An uncalled excess is returned even if its owner disconnected.
            for(int i=0;i<Capacity;++i) if(players_[i].committed>=level) payouts[i]+=amount/contributors;
            continue;
        }
        std::vector<int> winners; uint64_t best=0;
        for(int i:eligible) {
            auto cards=board_;cards.insert(cards.end(),players_[i].hole.begin(),players_[i].hole.end());
            uint64_t rank=eligible.size()==1 ? 1 : evaluate(cards);
            if(rank>best) { best=rank;winners.clear(); }
            if(rank==best) winners.push_back(i);
        }
        for(int i:winners) payouts[i]+=amount/static_cast<int64_t>(winners.size());
        int64_t odd=amount%static_cast<int64_t>(winners.size());
        for(int d=1;d<=Capacity && odd>0;++d) { int i=(dealer_+d)%Capacity;
            if(std::find(winners.begin(),winners.end(),i)!=winners.end()) { ++payouts[i];--odd; } }
    }
    std::ostringstream result;
    for(int i=0;i<Capacity;++i) { auto& p=players_[i]; p.chips+=payouts[i];p.bet=0;p.committed=0;p.pending=false;
        if(payouts[i]>0) result<<p.name<<" +"<<payouts[i]<<"  "; }
    result_=result.str();street_=protocol::COMPLETE;actor_=-1;currentBet_=0;
}
protocol::TableSnapshot Table::snapshot(const std::string& viewer) const {
    protocol::TableSnapshot s;s.set_table_id(1);s.set_hand_id(hand_);s.set_revision(revision_);s.set_street(street_);
    s.set_acting_seat(actor_);s.set_dealer_seat(dealer_);s.set_current_bet(currentBet_);s.set_min_raise(minRaise_);s.set_result(result_);
    for(auto c:board_) { auto out=s.add_board();out->set_rank(c.rank);out->set_suit(c.suit); }
    int64_t pot=0;
    for(int i=0;i<Capacity;++i) { auto& p=players_[i];if(p.id.empty()) continue;
        auto out=s.add_players();out->set_player_id(p.id);out->set_display_name(p.name);out->set_seat(i);out->set_chips(p.chips);
        out->set_street_bet(p.bet);out->set_committed(p.committed);out->set_folded(p.folded);out->set_ready(p.ready);out->set_connected(p.connected);
        if(p.id==viewer || (street_==protocol::COMPLETE && !p.folded && board_.size()==5))
            for(auto c:p.hole) { auto card=out->add_hole_cards();card->set_rank(c.rank);card->set_suit(c.suit); }
        pot+=p.committed;
    }
    s.set_pot(pot);return s;
}
}
