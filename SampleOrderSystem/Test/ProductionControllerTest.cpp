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

// T6-6: completeProduction — excessProduction → pureQty 귀속
//        qty=80, reqProd=100, shortage=50
//        excess = reqProd - shortage = 100 - 50 = 50
//        pureQty(승인 시 소진으로 0) + 50 = 50
TEST_F(ProductionControllerTest, CompleteProduction_ExcessToPureQty) {
    Order  o = makeOrderP("ORD0001", "S-001", 80, 100, OrderStatus::PRODUCING, 50);
    Sample s = makeSampleP("S-001", 0, 0.9, 1.2, 80);  // pureQty=0: 승인 시 소진됨
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.completeProduction("ORD0001");
    EXPECT_EQ(capturedSample.pureQuantity, 50); // 0 + (100 - 50)
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

#endif
