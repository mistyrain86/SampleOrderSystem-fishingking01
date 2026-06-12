#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/OrderController.h"
#include "Model/ISampleRepository.h"
#include "Model/IOrderRepository.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SaveArg;
using ::testing::NiceMock;

namespace {

class FakeClockO : public IClock {
public:
    explicit FakeClockO(std::string t) : time_(std::move(t)) {}
    std::string now() const override { return time_; }
private:
    std::string time_;
};

class MockOrderRepoOC : public IOrderRepository {
public:
    MOCK_METHOD(void,                 add,            (const Order&),               (override));
    MOCK_METHOD(std::optional<Order>, findById,       (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Order>,   findAll,        (),                   (const, override));
    MOCK_METHOD(std::vector<Order>,   findByStatus,   (OrderStatus),        (const, override));
    MOCK_METHOD(std::vector<Order>,   findBySampleId, (const std::string&), (const, override));
    MOCK_METHOD(bool,                 update,         (const Order&),               (override));
    MOCK_METHOD(int,                  count,          (),                   (const, override));
};

class MockSampleRepoOC : public ISampleRepository {
public:
    MOCK_METHOD(void,                  add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,   findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,   findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                  update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                  remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                   count,      (),                   (const, override));
};

static Sample makeSampleO(const std::string& id, int pureQty, double yield = 0.9,
                          double cycleTime = 0.5, int reservedQty = 0) {
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

static Order makeOrderO(const std::string& id, const std::string& sampleId,
                        int qty, OrderStatus status = OrderStatus::RESERVED) {
    Order o{};
    o.id                 = id;
    o.sampleId           = sampleId;
    o.customerName       = "테스트고객";
    o.quantity           = qty;
    o.status             = status;
    o.orderedAt          = "2026-06-12 09:00:00";
    o.requiredProduction = 0;
    return o;
}

} // namespace

// ========================================================
// calcRequiredProduction 정적 메서드 테스트
// ========================================================

// T5-1: 일반 케이스 — ceil(100 / (0.9 * 0.9)) = ceil(123.46) = 124
TEST(OrderControllerStaticTest, CalcRequired_Formula) {
    EXPECT_EQ(OrderController::calcRequiredProduction(100, 0.9), 124);
}

// T5-2: 경계값 — ceil(81 / (0.9 * 0.9)) = ceil(100.0) = 100
TEST(OrderControllerStaticTest, CalcRequired_EdgeCase) {
    EXPECT_EQ(OrderController::calcRequiredProduction(81, 0.9), 100);
}

// ========================================================
// OrderController 테스트 픽스처
// ========================================================
class OrderControllerTest : public ::testing::Test {
protected:
    NiceMock<MockOrderRepoOC>  orderRepo;
    NiceMock<MockSampleRepoOC> sampleRepo;
    FakeClockO                 clock{"2026-06-12 09:00:00"};
    OrderController            ctrl{orderRepo, sampleRepo, clock};
};

// ---- reserveOrder ----

// T5-3: 유효 sampleId → SUCCESS + add 1회 호출
TEST_F(OrderControllerTest, ReserveOrder_Success) {
    Sample s = makeSampleO("S-001", 200);
    EXPECT_CALL(sampleRepo, findById("S-001")).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  count()).WillOnce(Return(0));
    EXPECT_CALL(orderRepo,  add(_)).Times(1);

    EXPECT_EQ(ctrl.reserveOrder("S-001", "삼성전자", 100), ReserveResult::SUCCESS);
}

// T5-4: orderedAt = FakeClock 값으로 설정
TEST_F(OrderControllerTest, ReserveOrder_SetsOrderedAt) {
    Sample s = makeSampleO("S-001", 200);
    EXPECT_CALL(sampleRepo, findById("S-001")).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  count()).WillOnce(Return(0));

    Order captured{};
    EXPECT_CALL(orderRepo, add(_)).WillOnce(SaveArg<0>(&captured));

    ctrl.reserveOrder("S-001", "삼성전자", 100);
    EXPECT_EQ(captured.orderedAt, "2026-06-12 09:00:00");
}

// T5-5: 없는 sampleId → INVALID_SAMPLE_ID + add 미호출
TEST_F(OrderControllerTest, ReserveOrder_InvalidSampleId) {
    EXPECT_CALL(sampleRepo, findById("S-999")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(orderRepo,  add(_)).Times(0);

    EXPECT_EQ(ctrl.reserveOrder("S-999", "삼성전자", 100), ReserveResult::INVALID_SAMPLE_ID);
}

// ---- approveOrder 케이스 A ----

// T5-6: 재고 충분(500 >= 200) → SUCCESS_CONFIRMED + 상태 CONFIRMED
TEST_F(OrderControllerTest, ApproveOrder_CaseA_Confirmed) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 500);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    EXPECT_EQ(ctrl.approveOrder("ORD0001"), ApproveResult::SUCCESS_CONFIRMED);
    EXPECT_EQ(capturedOrder.status, OrderStatus::CONFIRMED);
}

// T5-7: 케이스 A — pureQty 감소 확인 (500 - 200 = 300)
TEST_F(OrderControllerTest, ApproveOrder_CaseA_PureQtyDecreases) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 500);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedSample.pureQuantity, 300);
}

// T5-8: 케이스 A — reservedQty 증가 확인 (50 + 200 = 250)
TEST_F(OrderControllerTest, ApproveOrder_CaseA_ReservedQtyIncreases) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 500, 0.9, 0.5, 50);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedSample.reservedQuantity, 250);
}

// ---- approveOrder 케이스 B ----

// T5-9: 재고 부족(30 < 200) → SUCCESS_PRODUCING + 상태 PRODUCING
TEST_F(OrderControllerTest, ApproveOrder_CaseB_Producing) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 30);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    EXPECT_EQ(ctrl.approveOrder("ORD0001"), ApproveResult::SUCCESS_PRODUCING);
    EXPECT_EQ(capturedOrder.status, OrderStatus::PRODUCING);
}

// T5-10: 케이스 B — reservedQty 즉시 선점 확인 (기존 pureQty=30만 reserved로 이동)
//         pureQty=30, 기존 reservedQty=10 → 10 + 30 = 40 (주문량 전체가 아닌 실보유분만)
TEST_F(OrderControllerTest, ApproveOrder_CaseB_ReservedQtyImmediate) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 30, 0.9, 0.5, 10);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedSample.reservedQuantity, 40); // 10(기존) + 30(pureQty) = 40
}

// T5-11: 케이스 B — pureQty 전량 소진 (이중 주문 방지)
//         pureQty=30 < qty=200 → 30 ea 모두 reserved 로 이동 → pureQty=0
TEST_F(OrderControllerTest, ApproveOrder_CaseB_PureQtyConsumed) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 30);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedSample.pureQuantity, 0);
}

// T5-12: 케이스 B — requiredProduction 산정 확인 (부족분 기준)
//         pureQty=30, qty=200, 부족분=170, yield=0.85
//         ceil(170 / (0.85 * 0.9)) = ceil(170 / 0.765) = ceil(222.22) = 223
TEST_F(OrderControllerTest, ApproveOrder_CaseB_RequiredProduction) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 30, 0.85);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedOrder.requiredProduction, 223);
}

// T5-12a: 케이스 B — productionShortage = qty - pureQty 저장 확인
//          pureQty=30, qty=200 → shortage=170
TEST_F(OrderControllerTest, ApproveOrder_CaseB_ProductionShortageStored) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 30);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedOrder.productionShortage, 170);
}

// T5-12b: 케이스 B — productionStartedAt = clock_.now() 로 설정
TEST_F(OrderControllerTest, ApproveOrder_CaseB_SetsProductionStartedAt) {
    Order  o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    Sample s = makeSampleO("S-001", 30);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));

    ctrl.approveOrder("ORD0001");
    EXPECT_EQ(capturedOrder.productionStartedAt, "2026-06-12 09:00:00");
}

// T5-13: RESERVED 아닌 주문 승인 시도 → FAILED
TEST_F(OrderControllerTest, ApproveOrder_NotReserved) {
    Order o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::CONFIRMED);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(orderRepo,  update(_)).Times(0);

    EXPECT_EQ(ctrl.approveOrder("ORD0001"), ApproveResult::FAILED);
}

// T5-14: 없는 orderId → FAILED
TEST_F(OrderControllerTest, ApproveOrder_NotFound) {
    EXPECT_CALL(orderRepo, findById("ORD9999")).WillOnce(Return(std::nullopt));

    EXPECT_EQ(ctrl.approveOrder("ORD9999"), ApproveResult::FAILED);
}

// ---- rejectOrder ----

// T5-15: RESERVED 주문 거절 → true + REJECTED 상태
TEST_F(OrderControllerTest, RejectOrder_Success) {
    Order o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    EXPECT_CALL(orderRepo, findById("ORD0001")).WillOnce(Return(o));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo, update(_)).WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));

    EXPECT_TRUE(ctrl.rejectOrder("ORD0001"));
    EXPECT_EQ(capturedOrder.status, OrderStatus::REJECTED);
}

// T5-16: 거절 후 재고 변동 없음 (sampleRepo.update 미호출)
TEST_F(OrderControllerTest, RejectOrder_NoInventoryChange) {
    Order o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::RESERVED);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);

    ctrl.rejectOrder("ORD0001");
}

// T5-17: RESERVED 아닌 주문 거절 시도 → false
TEST_F(OrderControllerTest, RejectOrder_NotReserved) {
    Order o = makeOrderO("ORD0001", "S-001", 200, OrderStatus::CONFIRMED);
    EXPECT_CALL(orderRepo, findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(orderRepo, update(_)).Times(0);

    EXPECT_FALSE(ctrl.rejectOrder("ORD0001"));
}

// ---- getReservedOrders / getOrderCount ----

// T5-18: getReservedOrders → RESERVED 주문만 반환
TEST_F(OrderControllerTest, GetReservedOrders_OnlyReserved) {
    std::vector<Order> reserved = {
        makeOrderO("ORD0001", "S-001", 100, OrderStatus::RESERVED),
        makeOrderO("ORD0002", "S-001", 200, OrderStatus::RESERVED)
    };
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::RESERVED)).WillOnce(Return(reserved));

    auto result = ctrl.getReservedOrders();
    EXPECT_EQ(result.size(), 2u);
}

// T5-19: getOrderCount — RELEASED·REJECTED 제외, 활성 주문만 카운트
TEST_F(OrderControllerTest, GetOrderCount_ExcludesReleasedAndRejected) {
    std::vector<Order> orders;
    auto makeO = [](const std::string& id, OrderStatus s) {
        Order o{}; o.id = id; o.status = s; return o;
    };
    orders.push_back(makeO("O1", OrderStatus::RESERVED));
    orders.push_back(makeO("O2", OrderStatus::PRODUCING));
    orders.push_back(makeO("O3", OrderStatus::CONFIRMED));
    orders.push_back(makeO("O4", OrderStatus::RELEASE));
    orders.push_back(makeO("O5", OrderStatus::REJECTED));
    EXPECT_CALL(orderRepo, findAll()).WillOnce(Return(orders));
    EXPECT_EQ(ctrl.getOrderCount(), 3); // RELEASED·REJECTED 2건 제외
}

#endif
