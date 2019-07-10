//
// BbrAdaptiveAdaptiveState.h
//
// State machine (and states) for TcpBbrAdaptive.
//

#ifndef BbrAdaptive_STATE_H
#define BbrAdaptive_STATE_H

#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

class TcpBbrAdaptive; 
class BbrAdaptiveState;

namespace BbrAdaptive {

// Defined BbrAdaptive' states.
enum BbrAdaptive_state {
  UNDEFINED_STATE=-1,
  STARTUP_STATE,
  DRAIN_STATE,
  PROBE_BW_STATE,
  PROBE_RTT_STATE,
};

} // end of namespace BbrAdaptive

///////////////////////////////////////////////
// BbrAdaptive' State Machine
class BbrAdaptiveStateMachine : public Object {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;

  // Constructor.
  BbrAdaptiveStateMachine();
  BbrAdaptiveStateMachine(TcpBbrAdaptive *owner);

  // Change state machine to new state.
  void changeState(BbrAdaptiveState *p_new_state);

  // Get type of current state.
  BbrAdaptive::BbrAdaptive_state getStateType() const;

  // Update by executing current state.
  void update();

 private:
  BbrAdaptiveState *m_state;           // Current state.
  TcpBbrAdaptive *m_owner;             // BbrAdaptive' flow that owns machine.
};

///////////////////////////////////////////////
// BbrAdaptive' State.
class BbrAdaptiveState : public Object {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  virtual std::string GetName() const;
  
  // Constructors.
  BbrAdaptiveState();
  BbrAdaptiveState(TcpBbrAdaptive *owner);

  // Destructor.
  virtual ~BbrAdaptiveState();

  // Get state type.
  virtual BbrAdaptive::BbrAdaptive_state getType() const=0;

  // Invoked when state first entered.
  virtual void enter();

  // Invoked when state updated.
  virtual void execute()=0;

  // Invoked when state exited.
  virtual void exit();

 protected:
  TcpBbrAdaptive *m_owner;             // BbrAdaptive' flow that owns state.
};

///////////////////////////////////////////////
// BbrAdaptive' STARTUP state

class BbrAdaptiveStartupState : public BbrAdaptiveState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrAdaptiveStartupState(TcpBbrAdaptive *owner);
  BbrAdaptiveStartupState();

  // Get state type.
  BbrAdaptive::BbrAdaptive_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  double m_full_bw;                        // Max prev BW in STARTUP.
  int m_full_bw_count;                     // Times BW not grown in STARTUP.
};

///////////////////////////////////////////////
// BbrAdaptive' DRAIN

class BbrAdaptiveDrainState : public BbrAdaptiveState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrAdaptiveDrainState(TcpBbrAdaptive *owner);
  BbrAdaptiveDrainState();

  // Get state type.
  BbrAdaptive::BbrAdaptive_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  uint32_t m_inflight_limit; // Target bytes in flight to exit DRAIN state.
  uint32_t m_round_count;    // Number of rounds in DRAIN state.
};

///////////////////////////////////////////////
// BbrAdaptive' PROBE_BW

class BbrAdaptiveProbeBWState : public BbrAdaptiveState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrAdaptiveProbeBWState(TcpBbrAdaptive *owner);
  BbrAdaptiveProbeBWState();

  // Get state type.
  BbrAdaptive::BbrAdaptive_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  int m_gain_cycle;                        // For cycling gain in PROBE_BW.
};

///////////////////////////////////////////////
// BbrAdaptive' PROBE_RTT

class BbrAdaptiveProbeRTTState : public BbrAdaptiveState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrAdaptiveProbeRTTState(TcpBbrAdaptive *owner);
  BbrAdaptiveProbeRTTState();

  // Get state type.
  BbrAdaptive::BbrAdaptive_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  Time m_probe_rtt_time;     // Time to remain in PROBE_RTT.
};

} // end of namespace ns3
#endif
