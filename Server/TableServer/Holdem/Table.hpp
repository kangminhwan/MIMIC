#pragma once
#include "mimic.pb.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mimic {
struct Card { int rank; int suit; };
uint64_t evaluate(const std::vector<Card>& cards);
struct Player {
    std::string id, name;
    int64_t chips = 1000, bet = 0, committed = 0;
    bool folded = false, ready = false, connected = true, pending = false;
    std::vector<Card> hole;
};
class Table {
public:
    static constexpr int Capacity = 6;
    void join(const std::string& id, const std::string& name);
    void leave(const std::string& id);
    void disconnect(const std::string& id);
    void ready(const std::string& id, bool value);
    void act(const std::string& id, const protocol::ActionRequest& request);
    protocol::TableSnapshot snapshot(const std::string& viewer) const;
    int count() const;
    bool contains(const std::string& id) const;
private:
    std::array<Player, Capacity> players_{};
    std::vector<Card> deck_, board_;
    protocol::Street street_ = protocol::WAITING;
    uint64_t hand_ = 0, revision_ = 0;
    int dealer_ = -1, actor_ = -1;
    int64_t currentBet_ = 0, minRaise_ = 20;
    std::string result_;
    bool active() const;
    int seat(const std::string& id) const;
    int next(int from, bool pendingOnly = false) const;
    void start();
    void pay(int seat, int64_t amount);
    void progress(int previous);
    void finish();
    Card draw();
};
}
