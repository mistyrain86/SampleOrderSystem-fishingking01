#include <gtest/gtest.h>
#include "Model/OrderStatus.h"
#include "Model/Sample.h"
#include "Model/Order.h"
#include "Util/Constants.h"
#include "Util/IClock.h"
#include "Util/SystemClock.h"

// ---- 테스트 전용 FakeClock (프로덕션 코드 오염 방지) ----
class FakeClock : public IClock {
public:
    explicit FakeClock(std::string fixedTime) : time_(std::move(fixedTime)) {}
    std::string now() const override { return time_; }
    void setTime(const std::string& t) { time_ = t; }
private:
    std::string time_;
};

// ---- OrderStatus toString ----
TEST(OrderStatus, ToString_Reserved)  { EXPECT_EQ(toString(OrderStatus::RESERVED),  "RESERVED"); }
TEST(OrderStatus, ToString_Rejected)  { EXPECT_EQ(toString(OrderStatus::REJECTED),  "REJECTED"); }
TEST(OrderStatus, ToString_Producing) { EXPECT_EQ(toString(OrderStatus::PRODUCING), "PRODUCING"); }
TEST(OrderStatus, ToString_Confirmed) { EXPECT_EQ(toString(OrderStatus::CONFIRMED), "CONFIRMED"); }
TEST(OrderStatus, ToString_Release)   { EXPECT_EQ(toString(OrderStatus::RELEASE),   "RELEASE"); }

// ---- Sample 필드 ----
TEST(Sample, TwoQuantityFieldsAreIndependent) {
    Sample s;
    s.pureQuantity     = 100;
    s.reservedQuantity = 50;
    EXPECT_EQ(s.pureQuantity,     100);
    EXPECT_EQ(s.reservedQuantity, 50);
}

TEST(Sample, CycleTimeField) {
    Sample s;
    s.cycleTime = 0.5; // min/ea
    EXPECT_DOUBLE_EQ(s.cycleTime, 0.5);
}

// ---- Order 기본값 ----
TEST(Order, DefaultRequiredProductionIsZero) {
    Order o{};
    EXPECT_EQ(o.requiredProduction, 0);
}

TEST(Order, DefaultStatusIsReserved) {
    Order o{};
    o.status = OrderStatus::RESERVED;
    EXPECT_EQ(o.status, OrderStatus::RESERVED);
}

// ---- Constants ----
TEST(Constants, ProductionSafetyFactor) {
    EXPECT_DOUBLE_EQ(PRODUCTION_SAFETY_FACTOR, 0.9);
}

TEST(Constants, MonitorPollIntervalSec) {
    EXPECT_EQ(MONITOR_POLL_INTERVAL_SEC, 5);
}

// ---- FakeClock ----
TEST(FakeClock, ReturnsFixedTime) {
    FakeClock clock("2026-06-12 09:00:00");
    EXPECT_EQ(clock.now(), "2026-06-12 09:00:00");
}

TEST(FakeClock, SetTimeChangesReturnValue) {
    FakeClock clock("2026-06-12 09:00:00");
    clock.setTime("2026-06-12 10:00:00");
    EXPECT_EQ(clock.now(), "2026-06-12 10:00:00");
}

TEST(FakeClock, PolymorphicUsageViaInterface) {
    FakeClock fake("2026-06-12 00:00:00");
    IClock& clock = fake;
    EXPECT_EQ(clock.now(), "2026-06-12 00:00:00");
}

// ---- SystemClock ----
TEST(SystemClock, ReturnsNonEmptyString) {
    SystemClock clock;
    std::string t = clock.now();
    EXPECT_EQ(t.size(), 19u); // "YYYY-MM-DD HH:MM:SS"
}

TEST(SystemClock, FormatContainsDashes) {
    SystemClock clock;
    std::string t = clock.now();
    EXPECT_EQ(t[4],  '-');
    EXPECT_EQ(t[7],  '-');
    EXPECT_EQ(t[10], ' ');
    EXPECT_EQ(t[13], ':');
    EXPECT_EQ(t[16], ':');
}
