#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <array>
#include <string>
#include <vector>
#include <limits>

using namespace geode::prelude;

namespace framewindow {

struct TimingBucket {
    const char* label;
    int minFrames;
    int maxFrames;
    unsigned count;
};

static std::array<TimingBucket, 10> makeBuckets() {
    return {{
        {"1", 1, 1, 0}, {"2", 2, 2, 0}, {"3", 3, 3, 0}, {"4", 4, 4, 0},
        {"5 - 6", 5, 6, 0}, {"7 - 8", 7, 8, 0}, {"9 - 12", 9, 12, 0},
        {"13 - 16", 13, 16, 0}, {"17 - 19", 17, 19, 0}, {"20+", 20, std::numeric_limits<int>::max(), 0}
    }};
}

struct InputEvent {
    uint64_t frame = 0;
    uint8_t button = 1;
    bool player2 = false;
    bool pressed = false;
};

struct TimingMarker {
    uint64_t sourceFrame = 0;
    int firstValidOffset = 0;
    int lastValidOffset = 0;
    cocos2d::CCPoint worldPosition = cocos2d::CCPointZero;
    bool verified = false;
};

static std::vector<InputEvent> g_macroInputs;
static std::vector<TimingMarker> g_markers;
static std::array<TimingBucket, 10> g_buckets = makeBuckets();
static bool g_showcase = false;
static bool g_markersVisible = true;
static cocos2d::CCNode* g_overlay = nullptr;

// TODO: Parse the selected .gdr2 file using the official GDReplayFormat spec.
// Do not assume that every GDR2 input record is just a frame delta; validate
// version, tags, lengths, platformer buttons, player number, and bounds.
static bool loadGDR2(std::string const& path) {
    (void)path;
    log::warn("[FrameWindow] .gdr2 parser is not implemented yet.");
    return false;
}

// Must be connected to actual GD per-frame simulation before any timing is
// labelled verified. Correct simulation needs mode, speed, gravity, portals,
// triggers, dual state, slopes, player hitboxes and exact frame semantics.
static bool testOffsetWithGamePhysics(PlayLayer* layer, InputEvent const& input,
                                      int offset, TimingMarker& result) {
    (void)layer;
    (void)input;
    (void)offset;
    (void)result;
    return false;
}

static void analyzeCurrentMacro(PlayLayer* layer) {
    g_markers.clear();
    g_buckets = makeBuckets();
    if (!layer) return;

    for (auto const& input : g_macroInputs) {
        if (!input.pressed) continue;
        TimingMarker marker;
        marker.sourceFrame = input.frame;

        for (int offset = -120; offset <= 120; ++offset) {
            TimingMarker candidate;
            if (testOffsetWithGamePhysics(layer, input, offset, candidate)) {
                if (!marker.verified) marker.firstValidOffset = offset;
                marker.lastValidOffset = offset;
                marker.worldPosition = candidate.worldPosition;
                marker.verified = true;
            }
        }

        if (!marker.verified) continue;
        int width = marker.lastValidOffset - marker.firstValidOffset + 1;
        for (auto& bucket : g_buckets) {
            if (width >= bucket.minFrames && width <= bucket.maxFrames) {
                ++bucket.count;
                break;
            }
        }
        g_markers.push_back(marker);
    }
}

// Temporary visual placeholder. Replace the generated dot with your own
// circle sprites later. Frame text is deliberately independent of the art.
static cocos2d::CCNode* createMarkerPlaceholder(TimingMarker const& marker) {
    auto root = cocos2d::CCNode::create();
    auto dot = cocos2d::CCDrawNode::create();
    dot->drawDot(cocos2d::CCPointZero, 15.f, cocos2d::ccc4f(0.67f, 0.38f, 1.f, 0.88f));
    root->addChild(dot);

    std::string text = marker.firstValidOffset == marker.lastValidOffset
        ? std::to_string(marker.firstValidOffset)
        : std::to_string(marker.firstValidOffset) + "-" + std::to_string(marker.lastValidOffset);
    auto label = cocos2d::CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    if (label) {
        label->setScale(0.32f);
        label->setPosition(cocos2d::CCPointZero);
        root->addChild(label);
    }
    return root;
}

// TODO: Call this at the correct level-frame / macro-frame update, not by
// wall-clock dt. Add sound playback at the same event boundary.
static void updateShowcaseAtFrame(PlayLayer* layer) {
    if (!layer || !g_showcase) return;
    if (!g_overlay) {
        g_overlay = cocos2d::CCNode::create();
        g_overlay->setID("framewindow-overlay"_spr);
        layer->addChild(g_overlay, 1000);
        for (auto const& marker : g_markers) {
            auto node = createMarkerPlaceholder(marker);
            node->setPosition(marker.worldPosition);
            g_overlay->addChild(node);
        }
    }
    // TODO: Track macro playback frame and trigger marker visibility/sounds.
}

static void toggleShowcase(PlayLayer* layer) {
    if (!layer) {
        FLAlertLayer::create("FrameWindow", "Open a level first.", "OK")->show();
        return;
    }
    g_showcase = !g_showcase;
    if (g_showcase) {
        analyzeCurrentMacro(layer);
        updateShowcaseAtFrame(layer);
    } else if (g_overlay) {
        g_overlay->removeFromParentAndCleanup(true);
        g_overlay = nullptr;
    }
}

} // namespace framewindow

class $modify(FrameWindowPlayLayer, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        framewindow::updateShowcaseAtFrame(this);
    }
};

class $modify(FrameWindowPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Showcase", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.7f),
            this,
            menu_selector(FrameWindowPauseLayer::onFrameWindowShowcase)
        );
        button->setID("framewindow-showcase-button"_spr);

        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(button);
            menu->updateLayout();
        } else {
            auto menu = CCMenu::create();
            menu->setID("framewindow-pause-menu"_spr);
            menu->addChild(button);
            menu->setPosition({CCDirector::sharedDirector()->getWinSize().width / 2.f,
                               CCDirector::sharedDirector()->getWinSize().height / 2.f});
            this->addChild(menu);
        }
    }

    void onFrameWindowShowcase(CCObject*) {
        framewindow::toggleShowcase(PlayLayer::get());
    }
};

class $modify(FrameWindowMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        log::info("[FrameWindow] Loaded. Physics/replay integration remains TODO.");
        return true;
    }
};
