#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace autojump {

struct Rect {
    float left = 0.f;
    float bottom = 0.f;
    float right = 0.f;
    float top = 0.f;
};

struct Obstacle {
    Rect bounds;
    bool hazard = true;
};

// This is a heuristic for static objects, not a physics simulator.
// All geometry is expressed in Geometry Dash world coordinates.
class Planner {
public:
    void clear() {
        m_obstacles.clear();
        m_sorted = true;
        m_lastTriggeredX = -std::numeric_limits<float>::infinity();
    }

    void add(Obstacle obstacle) {
        if (obstacle.bounds.right < obstacle.bounds.left ||
            obstacle.bounds.top < obstacle.bounds.bottom) return;
        m_obstacles.push_back(obstacle);
        m_sorted = false;
    }

    std::size_t obstacleCount() const { return m_obstacles.size(); }

    void newAttempt() {
        m_lastTriggeredX = -std::numeric_limits<float>::infinity();
    }

    bool shouldJump(
        Rect player, bool grounded, bool upsideDown, float lookahead,
        bool jumpOverSolids
    ) {
        if (!grounded || m_obstacles.empty()) return false;
        lookahead = std::clamp(lookahead, 8.f, 130.f);
        sortIfNeeded();
        // Query only the narrow strip ahead; no full level scan per frame.
        auto it = std::lower_bound(
            m_obstacles.begin(), m_obstacles.end(), player.left - 55.f,
            [](Obstacle const& o, float x) { return o.bounds.left < x; }
        );
        for (; it != m_obstacles.end(); ++it) {
            const Rect& b = it->bounds;
            if (b.left > player.right + lookahead) break;
            if (!it->hazard && !jumpOverSolids) continue;
            // Ignore long floor/ceiling platforms; focus on 1-block obstacles.
            if (!it->hazard && (b.right - b.left > 46.f || b.top - b.bottom > 46.f))
                continue;
            // A nearby hazard must actually intersect the ground-level path.
            // For upside-down cube levels, test against the player's ceiling edge.
            if (!upsideDown) {
                if (it->hazard) {
                    if (b.top < player.bottom - 4.f || b.bottom > player.bottom + 32.f)
                        continue;
                } else {
                    // Solids completely beneath the feet are floor, not an obstacle.
                    if (b.top <= player.bottom + 2.f || b.bottom > player.bottom + 32.f)
                        continue;
                }
            } else {
                if (it->hazard) {
                    if (b.bottom > player.top + 4.f || b.top < player.top - 32.f)
                        continue;
                } else {
                    if (b.bottom >= player.top - 2.f || b.top < player.top - 32.f)
                        continue;
                }
            }
            float gap = b.left - player.right;
            if (gap < -3.f || gap > lookahead) continue;
            // Never jump twice for the same obstacle in a single attempt.
            if (std::abs(b.left - m_lastTriggeredX) <= 2.f) continue;
            m_lastTriggeredX = b.left;
            return true;
        }
        return false;
    }

private:
    void sortIfNeeded() {
        if (m_sorted) return;
        std::stable_sort(m_obstacles.begin(), m_obstacles.end(),
            [](Obstacle const& a, Obstacle const& b) {
                return a.bounds.left < b.bounds.left;
            });
        m_sorted = true;
    }

    std::vector<Obstacle> m_obstacles;
    bool m_sorted = true;
    float m_lastTriggeredX = -std::numeric_limits<float>::infinity();
};

} // namespace autojump
