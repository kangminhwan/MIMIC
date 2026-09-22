#include "Holdem/Table.hpp"
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <set>
using namespace mimic;
using namespace mimic::protocol;
namespace {
int checks=0;
void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
void rejected(const std::function<void()>& action){bool failed=false;try{action();}catch(const std::runtime_error&){failed=true;}check(failed,"Expected rejection");}
ActionRequest action(const TableSnapshot& s,ActionKind kind,int64_t amount=0){ActionRequest r;r.set_hand_id(s.hand_id());r.set_revision(s.revision());r.set_kind(kind);r.set_raise_to(amount);return r;}
const PlayerState& actor(const TableSnapshot& s){for(auto& p:s.players())if(static_cast<int>(p.seat())==s.acting_seat())return p;throw std::runtime_error("Missing actor");}
void conservation(const TableSnapshot& s,int64_t expected){int64_t total=s.pot();for(auto& p:s.players()){check(p.chips()>=0,"Negative chips");total+=p.chips();}check(total==expected,"Chips not conserved");}
void ranks(){
    auto royal=evaluate({{14,0},{13,0},{12,0},{11,0},{10,0},{2,1},{3,1}});
    auto quads=evaluate({{14,0},{14,1},{14,2},{14,3},{13,0}});
    auto full=evaluate({{13,0},{13,1},{13,2},{12,0},{12,1}});
    auto flush=evaluate({{14,1},{11,1},{9,1},{6,1},{3,1}});
    auto straight=evaluate({{6,0},{5,1},{4,2},{3,0},{2,1}});
    auto wheel=evaluate({{14,0},{2,1},{3,2},{4,0},{5,1}});
    auto trips=evaluate({{12,0},{12,1},{12,2},{14,0},{3,1}});
    auto twopair=evaluate({{14,0},{14,1},{13,2},{13,0},{2,1}});
    auto pair=evaluate({{14,0},{14,1},{12,2},{11,0},{9,1}});
    auto high=evaluate({{14,0},{13,1},{12,2},{11,0},{9,1}});
    check(royal>quads && quads>full && full>flush && flush>straight && straight>wheel && wheel>trips && trips>twopair && twopair>pair && pair>high,"Rank order");
    check(evaluate({{14,0},{14,1},{13,2},{8,0},{4,1}})>evaluate({{14,2},{14,3},{12,1},{11,0},{9,1}}),"Pair kicker");
    check(evaluate({{14,0},{13,0},{12,0},{11,0},{10,0},{2,1},{3,1}})==evaluate({{14,0},{13,0},{12,0},{11,0},{10,0},{4,1},{5,1}}),"Board plays tie");
    rejected([]{evaluate({{14,0},{14,0},{12,0},{11,0},{10,0}});});
}
void validation(){
    Table t;rejected([&]{t.join("","x");});
    t.join("a","Alice");t.join("b","Bob");rejected([&]{t.join("a","Alice");});
    t.ready("a",true);t.ready("b",true);
    auto s=t.snapshot("a");check(s.street()==PREFLOP,"Hand starts");check(s.dealer_seat()==s.acting_seat(),"Heads-up dealer acts first");
    check(s.players(0).hole_cards_size()==2 && s.players(1).hole_cards_size()==0,"Private hole cards");
    check(t.snapshot("spectator").players(0).hole_cards_size()==0,"Spectator cannot see cards");
    auto current=actor(s).player_id();auto other=current=="a" ? "b" : "a";
    rejected([&]{t.act(other,action(s,CALL));});rejected([&]{t.act(current,action(s,CHECK));});
    rejected([&]{t.act(current,action(s,RAISE,21));});rejected([&]{t.act(current,action(s,RAISE,1001));});
    rejected([&]{t.leave(current);});rejected([&]{t.join("c","Charlie");});
    auto stale=action(s,CALL);stale.set_revision(0);rejected([&]{t.act(current,stale);});
    check(t.snapshot("a").revision()==s.revision(),"Rejected requests do not mutate state");
    t.act(current,action(s,FOLD));s=t.snapshot("a");check(s.street()==COMPLETE && s.pot()==0,"Fold resolves pot");conservation(s,2000);
    t.leave("a");check(t.count()==1,"Leave table");
    Table multi;for(int i=0;i<6;++i)multi.join(std::to_string(i),std::to_string(i));rejected([&]{multi.join("7","Seven");});
    for(int i=0;i<6;++i)multi.ready(std::to_string(i),true);
    auto m=multi.snapshot("0");check(m.dealer_seat()==0 && m.acting_seat()==3,"Six-max button and blinds");
    std::set<std::pair<int,int>> cards;
    for(int i=0;i<6;++i) { auto privateState=multi.snapshot(std::to_string(i)); for(auto& p:privateState.players()) if(p.player_id()==std::to_string(i)) for(auto& c:p.hole_cards()) cards.emplace(c.rank(),c.suit()); }
    check(cards.size()==12,"Unique hole cards");
}
void disconnects(){
    Table t;t.join("a","A");t.join("b","B");t.ready("a",true);t.ready("b",true);
    t.disconnect(actor(t.snapshot("a")).player_id());auto s=t.snapshot("a");check(s.street()==COMPLETE,"Disconnect folds");conservation(s,2000);
    Table allin;allin.join("a","A");allin.join("b","B");allin.ready("a",true);allin.ready("b",true);
    auto before=allin.snapshot("a");allin.act(actor(before).player_id(),action(before,RAISE,1000));
    allin.disconnect("a");auto pending=allin.snapshot("b");check(!pending.players(0).folded(),"Disconnected all-in remains live");
    allin.act(actor(pending).player_id(),action(pending,CALL));auto after=allin.snapshot("b");check(after.street()==COMPLETE && after.board_size()==5,"All-in runout");conservation(after,2000);
}
void simulations(){
    std::mt19937 rng(413);
    for(int trial=0;trial<150;++trial){
        Table t;int n=2+trial%5;for(int i=0;i<n;++i)t.join(std::to_string(i),std::to_string(i));
        for(int hand=0;hand<6;++hand){
            auto initial=t.snapshot("");bool canStart=true;for(auto& p:initial.players())if(p.chips()<20)canStart=false;if(!canStart)break;
            for(int i=0;i<n;++i)t.ready(std::to_string(i),true);
            int steps=0;
            while(t.snapshot("").street()!=COMPLETE){
                check(++steps<200,"Hand must terminate");auto s=t.snapshot("");conservation(s,n*1000);auto p=actor(s);
                ActionKind kind=p.street_bet()==s.current_bet()?CHECK:CALL;int64_t amount=0;
                int roll=rng()%10;int responders=0;for(auto& other:s.players())if(other.player_id()!=p.player_id() && !other.folded() && other.chips()>0)++responders;
                if(roll==0)kind=FOLD;
                else if(roll>=8 && responders>0 && p.chips()+p.street_bet()>=s.current_bet()+s.min_raise()) {kind=RAISE;amount=roll==9?p.chips()+p.street_bet():s.current_bet()+s.min_raise();}
                t.act(p.player_id(),action(s,kind,amount));
            }
            conservation(t.snapshot(""),n*1000);
        }
    }
}
}
int main(){try{ranks();validation();disconnects();simulations();std::cout<<"PASS "<<checks<<" checks: ranking, privacy, actions, disconnects, all-in runouts and chip conservation\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
