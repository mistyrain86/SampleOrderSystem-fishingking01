#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/ProductionController.h"
#include "Model/ISampleRepository.h"
#include "Model/IOrderRepository.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SaveArg;
using ::testing::NiceMock;

namespace {

class FakeClockP : public IClock {
public:
    explicit FakeClockP(std::string t) : time_(std::move(t)) {}
    std::string now() const override { return time_; }
private:
    std::string time_;
};

class MockOrderRepoPC : public IOrderRepository {
public:
    MOCK_METHOD(void,                 add,            (const Order&),               (override));
    MOCK_METHOD(std::optional<Order>, findById,       (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Order>,   findAll,        (),                   (const, override));
    MOCK_METHOD(std::vector<Order>,   findByStatus,   (OrderStatus),        (const, override));
    MOCK_METHOD(std::vector<Order>,   findBySampleId, (const std::string&), (const, override));
    MOCK_METHOD(bool,                 update,         (const Order&),               (override));
    MOCK_METHOD(int,                  count,          (),                   (const, override));
};

class MockSampleRepoPC : public ISampleRepository {
public:
    MOCK_METHOD(void,                  add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,   findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,   findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                  update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                  remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                   count,      (),                   (const, override));
};

static Sample makeSampleP(const std::string& id, int pureQty,
                          double yield = 0.9, double cycleTime = 0.5,
                          int reservedQty = 0) {
    Sample s{};
    s.id               = id;
    s.name             = "테스트시료";
    s.pureQuantity     = pureQty;
    s.reservedQuantity = reservedQty;
    s.yield            = yield;
    s.cycleTime        = cycleTime;
    s.registeredAt     = "2026-06-12 09:00:00";
    return s;
}

static Order makeOrderP(const std::string& id, const std::string& sampleId,
                        int qty, int reqProd,
                        OrderStatus status              = OrderStatus::PRODUCING,
                        int         productionShortage  = 0) {
    Order o{};
    o.id                  = id;
    o.sampleId            = sampleId;
    o.customerName        = "테스트고객";
    o.quantity            = qty;
    o.requiredProduction  = reqProd;
    o.productionShortage  = productionShortage;
    o.status              = status;
    o.orderedAt           = "2026-06-12 09:00:00";
    return o;
}

} // namespace

// ========================================================
// calcEstimatedTime 정적 메서드 테스트
// ========================================================

// T6-1: 일반 케이스 — 1.2 × 61 = 73.2
TEST(ProductionControllerStaticTest, CalcEstimatedTime_Formula) {
    EXPECT_DOUBLE_EQ(ProductionController::calcEstimatedTime(1.2, 61), 73.2);
}

// T6-2: 경계값 — cycleTime=0.5, reqProd=0 → 0.0
TEST(ProductionControllerStaticTest, CalcEstimatedTime_Zero) {
    EXPECT_DOUBLE_EQ(ProductionController::calcEstimatedTime(0.5, 0), 0.0);
}

// ========================================================
// ProductionController 테스트 픽스처
// ========================================================
class ProductionControllerTest : public ::testing::Test {
protected:
    NiceMock<MockOrderRepoPC>  orderRepo;
    NiceMock<MockSampleRepoPC> sampleRepo;
    FakeClockP                 clock{"2026-06-12 09:00:00"};
    ProductionController       ctrl{orderRepo, sampleRepo, clock};
};

// T6-3: getProductionQueue → findByStatus(PRODUCING) 위임
TEST_F(ProductionControllerTest, GetProductionQueue_OnlyProducing) {
    std::vector<Order> producing = {
        makeOrderP("ORD0001", "S-001", 80, 100),
        makeOrderP("ORD0002", "S-001", 150, 190)
    };
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(producing));

    auto result = ctrl.getProductionQueue();
    EXPECT_EQ(result.size(), 2u);
}

// T6-4: getProductionCount → findByStatus(PRODUCING).size()
TEST_F(ProductionControllerTest, GetProductionCount_Delegates) {
    std::vector<Order> producing = {
        makeOrderP("ORD0001", "S-001", 80, 100),
        makeOrderP("ORD0002", "S-001", 150, 190)
    };
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(producing));

    EXPECT_EQ(ctrl.getProductionCount(), 2);
}

// T6-5: completeProduction 성공 → CONFIRMED 상태
TEST_F(ProductionControllerTest, CompleteProduction_Success) {
    Order  o = makeOrderP("ORD0001", "S-001", 80, 100, OrderStatus::PRODUCING);
    Sample s = makeSampleP("S-001", 0, 0.9, 1.2, 80);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));

    EXPECT_TRUE(ctrl.completeProduction("ORD0001"));
    EXPECT_EQ(capturedOrder.status, OrderStatus::CONFIRMED);
}

// T6-6: completeProduction — 실시간 적립 없이 완료
//        qty=80, reqProd=100, shortage=50 → origReserved=30
//        reservedQty=80 (30 원래 + 50 생산 적립) → totalCredited=50, remaining=50
//        reservedRemaining=0 → toReserved=0, toPure=50 → pureQty=50
TEST_F(ProductionControllerTest, CompleteProduction_ExcessToPureQty) {
    Order  o = makeOrderP("ORD0001", "S-001", 80, 100, OrderStatus::PRODUCING, 50);
    Sample s = makeSampleP("S-001", 0, 0.9, 1.2, 80);  // pureQty=0, reservedQty=80
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.completeProduction("ORD0001");
    EXPECT_EQ(capturedSample.pureQuantity, 50);
}

// T6-6b: completeProduction — 실시간 부분 적립 후 완료
//         qty=80, reqProd=100, shortage=50 → origReserved=30
//         20개 먼저 생산 적립됨: reservedQty=50 (30+20), pureQty=0
//         remaining=80, reservedRemaining=30 → toReserved=30, toPure=50
//         → reservedQty=80, pureQty=50
TEST_F(ProductionControllerTest, CompleteProduction_WithPartialRealTimeCredit) {
    Order  o = makeOrderP("ORD0001", "S-001", 80, 100, OrderStatus::PRODUCING, 50);
    Sample s = makeSampleP("S-001", 0, 0.9, 1.2, 50); // pureQty=0, reservedQty=50 (20 생산 적립)
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.completeProduction("ORD0001");
    EXPECT_EQ(capturedSample.pureQuantity,     50);
    EXPECT_EQ(capturedSample.reservedQuantity, 80);
}

// T6-7: completeProduction — reservedQty 불변
TEST_F(ProductionControllerTest, CompleteProduction_ReservedUnchanged) {
    Order  o = makeOrderP("ORD0001", "S-001", 80, 100, OrderStatus::PRODUCING);
    Sample s = makeSampleP("S-001", 0, 0.9, 1.2, 80);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.completeProduction("ORD0001");
    EXPECT_EQ(capturedSample.reservedQuantity, 80); // 불변
}

// T6-8: PRODUCING 아닌 주문 완료 시도 → false
TEST_F(ProductionControllerTest, CompleteProduction_NotProducing) {
    Order o = makeOrderP("ORD0001", "S-001", 80, 100, OrderStatus::CONFIRMED);
    EXPECT_CALL(orderRepo, findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(orderRepo,  update(_)).Times(0);

    EXPECT_FALSE(ctrl.completeProduction("ORD0001"));
}

// T6-9: 없는 orderId → false
TEST_F(ProductionControllerTest, CompleteProduction_NotFound) {
    EXPECT_CALL(orderRepo, findById("ORD9999")).WillOnce(Return(std::nullopt));

    EXPECT_FALSE(ctrl.completeProduction("ORD9999"));
}

// ========================================================
// creditRealTimeExcess 테스트
// 시나리오: qty=30, shortage=22 → origReserved=8
//   승인 직후 DB 상태: reservedQty=8(원래 pureQty 이동), pureQty=0
// ========================================================

// T6-10: producedSoFar=10 (shortage 미만) → 전부 reservedQty 적립
//         productionCreditedToReserved=0, delta=10, toReserved=10, toPure=0
//         → reservedQty=18, pureQty=0
TEST_F(ProductionControllerTest, CreditRealTimeExcess_BelowShortageGoesToReserved) {
    Order  o = makeOrderP("ORD0001", "S-001", 30, 32, OrderStatus::PRODUCING, 22);
    Sample s = makeSampleP("S-001", 0, 0.9, 0.5, 8); // pureQty=0, reservedQty=8(원래 pureQty)
    NiceMock<MockOrderRepoPC>  niceOrderRepo;
    NiceMock<MockSampleRepoPC> niceSampleRepo;
    FakeClockP fakeClock("2026-06-12 09:00:00");
    ProductionController c(niceOrderRepo, niceSampleRepo, fakeClock);

    EXPECT_CALL(niceOrderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(niceSampleRepo, findById("S-001")).WillOnce(Return(s));

    Sample captured{};
    EXPECT_CALL(niceSampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    c.creditRealTimeExcess("ORD0001", 10);
    EXPECT_EQ(captured.reservedQuantity, 18); // 8+10
    EXPECT_EQ(captured.pureQuantity,      0);
}

// T6-11: producedSoFar=10, 이미 reservedQty=18(8+10 생산 적립) → delta=0 → update 안 함
TEST_F(ProductionControllerTest, CreditRealTimeExcess_NoDeltaNoUpdate) {
    Order  o = makeOrderP("ORD0001", "S-001", 30, 32, OrderStatus::PRODUCING, 22);
    Sample s = makeSampleP("S-001", 0, 0.9, 0.5, 18); // 이미 10 생산분 적립됨
    NiceMock<MockOrderRepoPC>  niceOrderRepo;
    NiceMock<MockSampleRepoPC> niceSampleRepo;
    FakeClockP fakeClock("2026-06-12 09:00:00");
    ProductionController c(niceOrderRepo, niceSampleRepo, fakeClock);

    EXPECT_CALL(niceOrderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(niceSampleRepo, findById("S-001")).WillOnce(Return(s));
    EXPECT_CALL(niceSampleRepo, update(_)).Times(0); // delta=0, 업데이트 없음

    c.creditRealTimeExcess("ORD0001", 10);
}

// T6-12: 이미 4개 생산 적립(reservedQty=12) → producedSoFar=10 → delta=6 추가
//         → reservedQty=18, pureQty=0
TEST_F(ProductionControllerTest, CreditRealTimeExcess_IncrementalDelta) {
    Order  o = makeOrderP("ORD0001", "S-001", 30, 32, OrderStatus::PRODUCING, 22);
    Sample s = makeSampleP("S-001", 0, 0.9, 0.5, 12); // 8(원래)+4(생산) = 12
    NiceMock<MockOrderRepoPC>  niceOrderRepo;
    NiceMock<MockSampleRepoPC> niceSampleRepo;
    FakeClockP fakeClock("2026-06-12 09:00:00");
    ProductionController c(niceOrderRepo, niceSampleRepo, fakeClock);

    EXPECT_CALL(niceOrderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(niceSampleRepo, findById("S-001")).WillOnce(Return(s));

    Sample captured{};
    EXPECT_CALL(niceSampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    c.creditRealTimeExcess("ORD0001", 10); // delta=10-4=6
    EXPECT_EQ(captured.reservedQuantity, 18); // 12+6
    EXPECT_EQ(captured.pureQuantity,      0);
}

#endif
