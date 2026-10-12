#include "../src/Planner.hpp"
#include <cassert>
#include <iostream>

using autojump::Planner;
using autojump::Rect;

static void tripleSpikeRegression();\nint main() {\n    tripleSpikeRegression();
    const Rect player{0, 0, 30, 30};
    {
        Planner bot;
        bot.add({{63, 0, 93, 30}, true});
        assert(bot.shouldJump(player, true, false, 47, true));
        assert(!bot.shouldJump(player, true, false, 47, true)); // no duplicate click
        bot.newAttempt();
        assert(bot.shouldJump(player, true, false, 47, true));
    }
    {
        Planner bot;
        bot.add({{120, 0, 150, 30}, true});
        assert(!bot.shouldJump(player, true, false, 47, true));
        assert(!bot.shouldJump(player, false, false, 130, true));
    }
    {
        Planner bot;
        bot.add({{60, 100, 90, 130}, true});
        assert(!bot.shouldJump(player, true, false, 47, true));
    }
    {
        Planner bot;
        bot.add({{60, -30, 90, 0}, false}); // normal floor
        assert(!bot.shouldJump(player, true, false, 47, true));
        bot.add({{60, 0, 90, 30}, false}); // one-block wall
        assert(!bot.shouldJump(player, true, false, 47, false));
        assert(bot.shouldJump(player, true, false, 47, true));
    }
    {
        Planner bot;
        bot.add({{65, 30, 95, 60}, true}); // upside-down target
        assert(bot.shouldJump(player, true, true, 47, true));
    }
    {
        Planner bot;
        bot.add({{70, 0, 100, 30}, true});
        bot.add({{61, 0, 91, 30}, true}); // added out of order
        assert(bot.shouldJump(player, true, false, 47, true));
    }
    std::cout << "planner tests passed\n";
}

// Triple-row regression: decision must be made before first spike.\nstatic void tripleSpikeRegression(){\n    autojump::Planner planner;\n    planner.add({{130,0,158,30},true});\n    planner.add({{160,0,188,30},true});\n    planner.add({{190,0,218,30},true});\n    autojump::Rect player{0,0,30,30};\n    auto scan=planner.sense(player,false,true);\n    assert(scan.cluster && scan.clusterEnd >= 218.f);\n    assert(planner.shouldJump(player,true,false,47.f,true));\n}\n