#if DEVELOPMENT_BUILD || UNITY_EDITOR
using System;
using System.Linq;
using System.Threading.Tasks;
using Mimic.Managers;
using Mimic.Protocol;
using UnityEngine;
namespace Mimic.Tests
{
    public sealed class RuntimeSmokeBot : MonoBehaviour
    {
        private async void Start()
        {
            string[] args = Environment.GetCommandLineArgs();
            int index = Array.IndexOf(args, "-mimic-smoke");
            if (index < 0 || index + 1 >= args.Length) { enabled = false; return; }
            try
            {
                var game = GetComponent<GaManager>();
                await game.Login(args[index + 1]);
                await game.Join(1);
                await game.Ready();
                float deadline = Time.realtimeSinceStartup + 45;
                ulong lastActedRevision = ulong.MaxValue;
                while (Time.realtimeSinceStartup < deadline)
                {
                    var state = game.Data.Table;
                    if (state != null && state.Street == Street.Complete)
                    {
                        if (state.Board.Count != 5 || state.Pot != 0 || state.Players.Count != 2 || state.Players.Sum(p => p.Chips) != 2000)
                            throw new Exception("Unity smoke settlement validation failed");
                        Debug.Log("MIMIC_UNITY_SMOKE_PASS " + args[index + 1] + " hand=" + state.HandId);
                        await Task.Delay(1500);
                        Application.Quit(0); return;
                    }
                    if (state != null && state.Street != Street.Waiting)
                    {
                        var own = state.Players.Single(p => p.PlayerId == game.Data.PlayerId);
                        if (own.HoleCards.Count != 2 || state.Players.Any(p => p.PlayerId != own.PlayerId && p.HoleCards.Count != 0))
                            throw new Exception("Unity smoke private card validation failed");
                        if ((int)own.Seat == state.ActingSeat && state.Revision != lastActedRevision)
                        {
                            lastActedRevision = state.Revision;
                            await game.Act(own.StreetBet == state.CurrentBet ? ActionKind.Check : ActionKind.Call);
                        }
                    }
                    await Task.Delay(50);
                }
                throw new TimeoutException("Unity smoke timed out");
            }
            catch (Exception error) { Debug.LogError("MIMIC_UNITY_SMOKE_FAIL " + error); Application.Quit(1); }
        }
    }
}
#endif
