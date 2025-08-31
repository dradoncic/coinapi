#pragma once 
#include <vector>
#include <string>
#include <unordered_map>
#include <enums/order_side.h>

struct LevelEvent 
{
    std::string time;
    double size;
    double price;
    Side side;
};

class LevelTape {
public:
    void log_event(const LevelEvent& event);
    const std::vector<LevelEvent>& get_events() const;

private:
    std::unordered_map<std::string, std::vector<LevelEvent>> events_;
};