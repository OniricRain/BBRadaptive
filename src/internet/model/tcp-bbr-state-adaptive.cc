//
// BbrState.cpp
//
// State machine and states for TcpBbr.
//

// Include files.
#include "ns3/log.h"
#include "tcp-bbr-adaptive.h"
#include "tcp-bbr-state-adaptive.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BbrAdaptiveState");
NS_OBJECT_ENSURE_REGISTERED(BbrAdaptiveState);

///////////////////////////////////////////////
// BbrAdaptive' State Machine
// Executes current state through update().
// Changes state through exit() and enter().

// Update state: STARTUP, DRAIN, PROBE_BW, PROBE_RTT.
//
//  State transition diagram:
//          |
//          V
//       STARTUP  
//          |     
//          V     
//        DRAIN   
//          |     
//          V     
// +---> PROBE_BW ----+
// |      ^    |      |
// |      |    |      |
// |      +----+      |
// |                  |
// +---- PROBE_RTT <--+

// Default constructor.
BbrAdaptiveStateMachine::BbrAdaptiveStateMachine() {
  NS_LOG_FUNCTION(this);
  m_owner = NULL;
  m_state = NULL;
}

// Default constructor.
BbrAdaptiveStateMachine::BbrAdaptiveStateMachine(TcpBbrAdaptive *owner) {
  NS_LOG_FUNCTION(this);
  m_owner = owner;
  m_state = NULL;
}

// Get type id.
TypeId BbrAdaptiveStateMachine::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrAdaptiveStateMachine")
    .SetParent<Object>()
    .SetGroupName("Internet")
    .AddConstructor<BbrAdaptiveStateMachine>();
  return tid;
}

// Get name of object.
std::string BbrAdaptiveStateMachine::GetName() const {
  NS_LOG_FUNCTION(this);
  return "BbrAdaptiveStateMachine";
}

// Get type of current state.
BbrAdaptive::BbrAdaptive_state BbrAdaptiveStateMachine::getStateType() const {
  return m_state -> getType();
}

// Update by executing current state.
void BbrAdaptiveStateMachine::update() {
  NS_LOG_FUNCTION(this);

  if (m_state == NULL) {
    NS_LOG_INFO(this << " m_state NULL. Probably flow terminated, so ok.");
    return;
  }

  NS_LOG_LOGIC(this << "  State: " << m_state -> GetName());

  // Check if should enter PROBE_RTT.
  if (m_owner -> checkProbeRTT())
    changeState(&m_owner -> m_state_probe_rtt);

  // Execute current state.
  m_state -> execute();

  // Cull RTT window.
  m_owner -> cullRTTwindow();

  // Cull BW window (except in DRAIN state).
  m_owner -> cullBWwindow();

  // Schedule next event (if we can).
  Time rtt = m_owner -> getRTT();
  if (!rtt.IsNegative()) {
    Simulator::Schedule(rtt, &BbrAdaptiveStateMachine::update, this);
    NS_LOG_LOGIC(this << "  Next event: " << rtt.GetSeconds());
  } else // update() will be called in PktsAcked() upon getting first rtt.
    NS_LOG_LOGIC(this << "  Not scheduling next event.");
}

// Change current state to new state.
void BbrAdaptiveStateMachine::changeState(BbrAdaptiveState *new_state) {
  NS_LOG_FUNCTION(this);
  NS_ASSERT(new_state != NULL);
  if (m_state)
    NS_LOG_LOGIC(this <<
		"  Old: " << m_state -> GetName() <<
		"  New: " << new_state -> GetName());
  else
    NS_LOG_LOGIC(this << " Initial state: " << new_state -> GetName());

  // Call exit on old state.
  if (m_state)
    m_state -> exit();

  // Change to new state.
  m_state = new_state;

  // Call enter on new state.
  m_state -> enter();
}

///////////////////////////////////////////////
// BbrAdaptive' State

BbrAdaptiveState::BbrAdaptiveState(TcpBbrAdaptive *owner) {
  m_owner = owner;
}

BbrAdaptiveState::BbrAdaptiveState() {
  m_owner = NULL;
}

BbrAdaptiveState::~BbrAdaptiveState() {
}

// Get type id.
TypeId BbrAdaptiveState::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrAdaptiveState")
    .SetParent<BbrAdaptiveStateMachine>()
    .SetGroupName("Internet");
  return tid;
}

// Get name of object.
std::string BbrAdaptiveState::GetName() const {
  return "BbrAdaptiveState";
}

// Invoked when state first entered.
void BbrAdaptiveState::enter() {
}

// Invoked when state exited.
void BbrAdaptiveState::exit() {
}

///////////////////////////////////////////////
// BbrAdaptive' STARTUP
  
BbrAdaptiveStartupState::BbrAdaptiveStartupState(TcpBbrAdaptive *owner) : BbrAdaptiveState(owner),
  m_full_bw(0),
  m_full_bw_count(0) {
  NS_LOG_FUNCTION(this);
}

BbrAdaptiveStartupState::BbrAdaptiveStartupState() : BbrAdaptiveState(),
  m_full_bw(0),
  m_full_bw_count(0) {
  NS_LOG_FUNCTION(this);
}

// Get type id.
TypeId BbrAdaptiveStartupState::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrAdaptiveStartupState")
    .SetParent<BbrAdaptiveStateMachine>()
    .SetGroupName("Internet")
    .AddConstructor<BbrAdaptiveStartupState>();
  return tid;
}

// Get name of object.
std::string BbrAdaptiveStartupState::GetName() const {
  return "BbrAdaptiveStartupState";
}

// Get state type.
BbrAdaptive::BbrAdaptive_state BbrAdaptiveStartupState::getType(void) const {
  return BbrAdaptive::STARTUP_STATE;
}

// Invoked when state first entered.
void BbrAdaptiveStartupState::enter() {
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO(this << " State: " << GetName());

  // Set gains to 2/ln(2).
  m_owner -> m_pacing_gain = BbrAdaptive::STARTUP_GAIN;
  m_owner -> m_cwnd_gain = BbrAdaptive::STARTUP_GAIN;
}

// Invoked when state updated.
void BbrAdaptiveStartupState::execute() {
  NS_LOG_FUNCTION(this);
  NS_LOG_LOGIC(this << " State: " << GetName());

  double new_bw = m_owner -> getBW();

  // If no legitimate estimates yet, no more to do.
  if (new_bw < 0) {
    NS_LOG_LOGIC(this << "  No BW estimates yet.");
    return;
  }
  
  // Still growing?
  if (new_bw > m_full_bw * BbrAdaptive::STARTUP_THRESHOLD) { 
    NS_LOG_LOGIC(this << "  Still growing. old_bw: " << m_full_bw << "  new_bw: " << new_bw);
    m_full_bw = new_bw;
    m_full_bw_count = 0;
    return;
  }

  // Another round w/o much growth.
  m_full_bw_count++;
  NS_LOG_LOGIC(this << "  Growth stalled. old_bw: " << m_full_bw << "  new_bw: " << new_bw << "  full-bw-count: " << m_full_bw_count);
  
  // If 3+ rounds w/out much growth, STARTUP --> DRAIN.
  if (m_full_bw_count > 2) {
    NS_LOG_LOGIC(this << "  Exiting STARTUP, next state DRAIN");
    m_owner -> m_machine.changeState(&m_owner -> m_state_drain);
  }

  return;
}

///////////////////////////////////////////////
// BbrAdaptive' DRAIN
  
BbrAdaptiveDrainState::BbrAdaptiveDrainState(TcpBbrAdaptive *owner) :
  BbrAdaptiveState(owner),
  m_inflight_limit(0),
  m_round_count(0) {
  NS_LOG_FUNCTION(this);
}

BbrAdaptiveDrainState::BbrAdaptiveDrainState() :
  BbrAdaptiveState(),
  m_inflight_limit(0),
  m_round_count(0) {
  NS_LOG_FUNCTION(this);
}

// Get type id.
TypeId BbrAdaptiveDrainState::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrAdaptiveDrainState")
    .SetParent<BbrAdaptiveStateMachine>()
    .SetGroupName("Internet")
    .AddConstructor<BbrAdaptiveDrainState>();
  return tid;
}

// Get name of object.
std::string BbrAdaptiveDrainState::GetName() const {
  return "BbrAdaptiveDrainState";
}

// Get state type.
BbrAdaptive::BbrAdaptive_state BbrAdaptiveDrainState::getType(void) const {
  return BbrAdaptive::DRAIN_STATE;
}

// Invoked when state first entered.
void BbrAdaptiveDrainState::enter() {
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO(this << " State: " << GetName());

  // Set pacing gain to 1/[2/ln(2)].
  m_owner -> m_pacing_gain = 1 / BbrAdaptive::STARTUP_GAIN;

  // Maintain high cwnd gain.
  if (PACING_CONFIG == NO_PACING)
    m_owner -> m_cwnd_gain = 1 / BbrAdaptive::STARTUP_GAIN; // Slow cwnd if no pacing.
  else
    m_owner -> m_cwnd_gain = BbrAdaptive::STARTUP_GAIN; // Maintain high cwnd gain.

  // Get BDP for target inflight limit when will exit STARTUUP..
  double bdp = m_owner -> getBDP();
  bdp = bdp * 1000000 / 8; // Convert to bytes.
  m_inflight_limit = (uint32_t) bdp;  
  m_round_count = 0;
}

// Invoked when state updated.
void BbrAdaptiveDrainState::execute() {
  NS_LOG_FUNCTION(this);
  NS_LOG_LOGIC(this << " State: " << GetName());

  NS_LOG_LOGIC(this << " " <<
	      GetName() <<
	      "  round: " << m_round_count <<
	      "  bytes_in_flight: " << m_owner -> m_bytes_in_flight <<
	      "  inflight_limit: " << m_inflight_limit);

  // See if should exit DRAIN state.
  // Do when byte-in-flight are under limit or 5 rounds
  // have passed (2.89/(1-1/2.89) ~ 4.5), whichever is first.
  m_round_count++;
  if (m_owner -> m_bytes_in_flight < m_inflight_limit ||
      m_round_count == 5) {
    NS_LOG_LOGIC(this << " Exiting DRAIN, next state PROBE_BW");
    m_owner -> m_machine.changeState(&m_owner -> m_state_probe_bw);
  }
}

///////////////////////////////////////////////
// BbrAdaptive' PROBE_BW
  
BbrAdaptiveProbeBWState::BbrAdaptiveProbeBWState(TcpBbrAdaptive *owner) : BbrAdaptiveState(owner) {
  NS_LOG_FUNCTION(this);
}

BbrAdaptiveProbeBWState::BbrAdaptiveProbeBWState() : BbrAdaptiveState() {
  NS_LOG_FUNCTION(this);
}

// Get type id.
TypeId BbrAdaptiveProbeBWState::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrAdaptiveProbeBWState")
    .SetParent<BbrAdaptiveStateMachine>()
    .SetGroupName("Internet")
    .AddConstructor<BbrAdaptiveProbeBWState>();
  return tid;
}

// Get name of object.
std::string BbrAdaptiveProbeBWState::GetName() const {
  return "BbrAdaptiveProbeBWState";
}

// Get state type.
BbrAdaptive::BbrAdaptive_state BbrAdaptiveProbeBWState::getType(void) const {
  return BbrAdaptive::PROBE_BW_STATE;
}

// Invoked when state first entered.
void BbrAdaptiveProbeBWState::enter() {
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO(this << " State: " << GetName());

  // Pick random start cycle phase (except "low") to avoid synch of
  // flows that enter PROBE_BW simultaneously.
  do {
    m_gain_cycle = rand() % 8;
  } while (m_gain_cycle == 1);  // Phase 1 is "low" cycle.

  NS_LOG_LOGIC(this << " " << GetName() << " Start cycle: " << m_gain_cycle);

  // Set gains based on phase.
  m_owner -> m_pacing_gain = BbrAdaptive::STEADY_FACTOR;
  if (m_gain_cycle == 0) // Phase 0 is "high" cycle.
    m_owner -> m_pacing_gain += BbrAdaptive::PROBE_FACTOR;
  if (PACING_CONFIG == NO_PACING)
    m_owner -> m_cwnd_gain = m_owner -> m_pacing_gain;
  else
    m_owner -> m_cwnd_gain = BbrAdaptive::STEADY_FACTOR * 2;
}

// Invoked when state updated.
void BbrAdaptiveProbeBWState::execute() {
  NS_LOG_FUNCTION(this);
  NS_LOG_LOGIC(this << " " << GetName() << "  m_gain_cycle: " << m_gain_cycle);

  // Set gain rate: [high, low, stdy, stdy, stdy, stdy, stdy, stdy]
  if (m_gain_cycle == 0)
    m_owner -> m_pacing_gain = BbrAdaptive::STEADY_FACTOR + BbrAdaptive::PROBE_FACTOR;
  else if (m_gain_cycle == 1)
    if (PACING_CONFIG == NO_PACING) 
      m_owner -> m_pacing_gain = BbrAdaptive::STEADY_FACTOR - BbrAdaptive::DRAIN_FACTOR/8;
    else
      m_owner -> m_pacing_gain = BbrAdaptive::STEADY_FACTOR - BbrAdaptive::DRAIN_FACTOR;
  else
    m_owner -> m_pacing_gain = BbrAdaptive::STEADY_FACTOR;

  if (PACING_CONFIG == NO_PACING)
    // If configed for NO_PACING, rate is controlled by cwnd at bdp.
    m_owner -> m_cwnd_gain = m_owner -> m_pacing_gain;
  else
    // Otherwise, cwnd can be twice bdp.
    m_owner -> m_cwnd_gain = 2 * BbrAdaptive::STEADY_FACTOR;

  // Move to next cycle, wrapping.
  m_gain_cycle++;
  if (m_gain_cycle > 7) //TODO: change length
    m_gain_cycle = 0;

  NS_LOG_LOGIC(this << " " <<
	      GetName() << " DATA pacing-gain: " << m_owner -> m_pacing_gain);
}

///////////////////////////////////////////////
// BbrAdaptive' PROBE_RTT
  
BbrAdaptiveProbeRTTState::BbrAdaptiveProbeRTTState(TcpBbrAdaptive *owner) : BbrAdaptiveState(owner) {
  NS_LOG_FUNCTION(this);
}

BbrAdaptiveProbeRTTState::BbrAdaptiveProbeRTTState() : BbrAdaptiveState() {
  NS_LOG_FUNCTION(this);
}

// Get type id.
TypeId BbrAdaptiveProbeRTTState::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrAdaptiveProbeRTTState")
    .SetParent<BbrAdaptiveStateMachine>()
    .SetGroupName("Internet")
    .AddConstructor<BbrAdaptiveProbeRTTState>();
  return tid;
}

// Get name of object.
std::string BbrAdaptiveProbeRTTState::GetName() const {
  return "BbrAdaptiveProbeRTTState";
}

// Get state type.
BbrAdaptive::BbrAdaptive_state BbrAdaptiveProbeRTTState::getType(void) const {
  return BbrAdaptive::PROBE_RTT_STATE;
}

// Invoked when state first entered.
void BbrAdaptiveProbeRTTState::enter() {
  NS_LOG_FUNCTION(this);
  NS_LOG_LOGIC(this << " State: " << GetName());

  // Set gains (Send() will minimize window);
  m_owner -> m_pacing_gain = BbrAdaptive::STEADY_FACTOR;
  m_owner -> m_cwnd_gain = BbrAdaptive::STEADY_FACTOR;

  // Compute time when to exit: max (0.2 seconds, min RTT).
  Time rtt = m_owner -> getRTT();
  if (rtt.GetSeconds() > 0.2)
    m_probe_rtt_time = rtt;
  else
    m_probe_rtt_time = Time(0.2 * 1000000000);
  m_probe_rtt_time = m_probe_rtt_time + Simulator::Now();
    
  NS_LOG_LOGIC(this << " " <<
	      GetName() << " In PROBE_RTT until: " << m_probe_rtt_time.GetSeconds());
}

// Invoked when state updated.
void BbrAdaptiveProbeRTTState::execute() {
  NS_LOG_FUNCTION(this);
  NS_LOG_LOGIC(this << " State: " << GetName());

  // Cwnd target is minimum.
  m_owner -> m_cwnd = BbrAdaptive::MIN_CWND * 1500; // In bytes.

  // If enough time elapsed, PROBE_RTT --> PROBE_BW.
  Time now = Simulator::Now();
  if (now > m_probe_rtt_time) {
      NS_LOG_LOGIC(this << " Exiting PROBE_RTT, next state PROBE_BW");
      m_owner -> m_machine.changeState(&m_owner -> m_state_probe_bw);
  }
}
