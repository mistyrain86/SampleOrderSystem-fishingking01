#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/MonitoringController.h"
#include "Model/ISampleRepository.h"
#include "Model/IOrderRepository.h"

using ::testing::Return;
using ::testing::NiceMock;

namespace {

class MockOrderRepoM : public IOrderRepository {
public:
    MOCK_METHOD(void,                 add,            (const Order&),               (override));
    MOCK_METHOD(std::optional<Order>, findById,       (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Order>,   findAll,        (),                   (const, override));
    MOCK_METHOD(std::vector<Order>,   findByStatus,   (OrderStatus),        (const, override));
    MOCK_METHOD(std::vector<Order>,   findBySampleId, (const std::string&), (const, override));
    MOCK_METHOD(bool,                 update,         (const Order&),               (override));
    MOCK_METHOD(int,                  count,          (),                   (const, override));
};

class MockSampleRepoM : public ISampleRepository {
public:
    MOCK_METHOD(void,                  add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,   findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,   findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                  update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                  remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                   count,      (),                   (const, override));
};

static Order makeOrderM(const std::string& id, const std::string& sampleId,
                        int qty, OrderStatus status) {
    Order o{};
    o.id           = id;
    o.sampleId     = sampleId;
    o.customerName = "테스트고객";
    o.quantity     = qty;
    o.status       = status;
    o.orderedAt    = "2026-06-12 09:00:00";
    return o;
}

static Sample makeSampleM(const std::string& id, int pureQty, int reservedQty = 0) {
    Sample s{};
    s.id               = id;
    s.name             = "테스트시료" + id;
    s.pureQuantity     = pureQty;
    s.reservedQuantity = reservedQty;
    s.yield            = 0.9;
    s.cycleTime        = 0.5;
    s.registeredAt     = "2026-06-12 09:00:00";
    return s;
}

} // namespace

// ========================================================
// MonitoringController 테스트 픽스처
// ========================================================
class MonitoringControllerTest : public ::testing::Test {
protected:
    NiceMock<MockOrderRepoM>  orderRepo;
    NiceMock<MockSampleRepoM> sampleRepo;
    MonitoringController      ctrl{orderRepo, sampleRepo};
};

// T7-1: 상태별 카운트 정확성
TEST_F(MonitoringControllerTest, GetOrderSummary_CountByStatus) {
    std::vector<Order> orders = {
        makeOrderM("O1", "S-001", 10, OrderStatus::RESERVED),
        makeOrderM("O2", "S-001", 10, OrderStatus::RESERVED),
        makeOrderM("O3", "S-001", 10, OrderStatus::PRODUCING),
        makeOrderM("O4", "S-001", 10, OrderStatus::CONFIRMED),
        makeOrderM("O5", "S-001", 10, OrderStatus::CONFIRMED),
        makeOrderM("O6", "S-001", 10, OrderStatus::CONFIRMED),
        makeOrderM("O7", "S-001", 10, OrderStatus::RELEASE),
        makeOrderM("O8", "S-001", 10, OrderStatus::RELEASE),
        makeOrderM("O9", "S-001", 10, OrderStatus::RELEASE),
        makeOrderM("O10","S-001", 10, OrderStatus::RELEASE),
    };
    EXPECT_CALL(orderRepo, findAll()).WillOnce(Return(orders));

    auto s = ctrl.getOrderSummary();
    EXPECT_EQ(s.reserved,  2);
    EXPECT_EQ(s.producing, 1);
    EXPECT_EQ(s.confirmed, 3);
    EXPECT_EQ(s.released,  4);
}

// T7-2: REJECTED 주문 집계 제외
TEST_F(MonitoringControllerTest, GetOrderSummary_ExcludesRejected) {
    std::vector<Order> orders = {
        makeOrderM("O1", "S-001", 10, OrderStatus::RESERVED),
        makeOrderM("O2", "S-001", 10, OrderStatus::REJECTED),
        makeOrderM("O3", "S-001", 10, OrderStatus::REJECTED),
    };
    EXPECT_CALL(orderRepo, findAll()).WillOnce(Return(orders));

    auto s = ctrl.getOrderSummary();
    EXPECT_EQ(s.reserved,  1);
    EXPECT_EQ(s.producing, 0);
    EXPECT_EQ(s.confirmed, 0);
    EXPECT_EQ(s.released,  0);
}

// T7-3: 재고 상태 — 여유 (pureQty >= RESERVED 주문 합계)
TEST_F(MonitoringControllerTest, GetInventorySummary_StatusSufficient) {
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(std::vector<Sample>{makeSampleM("S-001", 100)}));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{makeOrderM("O1", "S-001", 50, OrderStatus::RESERVED)}));

    auto result = ctrl.getInventorySummary();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].stockLevel, StockLevel::SUFFICIENT);
}

// T7-4: 재고 상태 — 부족 (0 < pureQty < RESERVED 주문 합계)
TEST_F(MonitoringControllerTest, GetInventorySummary_StatusLow) {
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(std::vector<Sample>{makeSampleM("S-001", 30)}));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{makeOrderM("O1", "S-001", 80, OrderStatus::RESERVED)}));

    auto result = ctrl.getInventorySummary();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].stockLevel, StockLevel::LOW);
}

// T7-5: 재고 상태 — 고갈 (pureQty == 0)
TEST_F(MonitoringControllerTest, GetInventorySummary_StatusDepleted) {
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(std::vector<Sample>{makeSampleM("S-001", 0, 50)}));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{}));

    auto result = ctrl.getInventorySummary();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].stockLevel, StockLevel::DEPLETED);
}

// T7-6: 잔여율 = pureQty / (pureQty + reservedQty) × 100
TEST_F(MonitoringControllerTest, GetInventorySummary_RemainRate) {
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(std::vector<Sample>{makeSampleM("S-001", 480, 120)}));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{}));

    auto result = ctrl.getInventorySummary();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0].remainRate, 80.0);
}

// T7-7: pureQty=0, reservedQty=0 → 잔여율 0.0 (0 나누기 방지)
TEST_F(MonitoringControllerTest, GetInventorySummary_ZeroTotal) {
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(std::vector<Sample>{makeSampleM("S-001", 0, 0)}));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{}));

    auto result = ctrl.getInventorySummary();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0].remainRate, 0.0);
}

// T7-8: 수량 0인 시료 포함, 전체 시료 수 = 결과 벡터 크기
TEST_F(MonitoringControllerTest, GetInventorySummary_AllSamplesIncluded) {
    std::vector<Sample> samples = {
        makeSampleM("S-001", 480),
        makeSampleM("S-002",  30),
        makeSampleM("S-003",   0),
    };
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(samples));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{}));

    auto result = ctrl.getInventorySummary();
    EXPECT_EQ(result.size(), 3u);
}

// T7-9: RESERVED 상태 주문만 합산 (CONFIRMED 등 다른 상태 제외)
TEST_F(MonitoringControllerTest, GetInventorySummary_OnlyReservedOrdersCount) {
    // pureQty=100, RESERVED 주문 50 → 여유 (CONFIRMED 200은 판정에 무관)
    EXPECT_CALL(sampleRepo, findAll()).WillOnce(Return(std::vector<Sample>{makeSampleM("S-001", 100)}));
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED))
        .WillOnce(Return(std::vector<Order>{makeOrderM("O1", "S-001", 50, OrderStatus::RESERVED)}));

    auto result = ctrl.getInventorySummary();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].stockLevel, StockLevel::SUFFICIENT); // CONFIRMED 200 무시
}

#endif
