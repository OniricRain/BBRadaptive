//
// BbrStateAdaptive.h
//
// State machine (and states) for TcpBbrAdaptive.
//

#ifndef BBR_STATE_H
#define BBR_STATE_H

#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

class TcpBbrAdaptive; 
class BbrStateAdaptive;

namespace bbra {

// Defined BBR' states.
enum bbr_state {
  UNDEFINED_STATE=-1,
  STARTUP_STATE,
  DRAIN_STATE,
  PROBE_BW_STATE,
  PROBE_RTT_STATE,
};

} // end of namespace bbr

///////////////////////////////////////////////
// BBR' State Machine
class BbrStateAdaptiveMachine : public Object {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;

  // Constructor.
  BbrStateAdaptiveMachine();
  BbrStateAdaptiveMachine(TcpBbrAdaptive *owner);

  // Change state machine to new state.
  void changeState(BbrStateAdaptive *p_new_state);

  // Get type of current state.
  bbra::bbr_state getStateType() const;

  // Update by executing current state.
  void update();

 private:
  BbrStateAdaptive *m_state;           // Current state.
  TcpBbrAdaptive *m_owner;             // BBR' flow that owns machine.
};

///////////////////////////////////////////////
// BBR' State.
class BbrStateAdaptive : public Object {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  virtual std::string GetName() const;
  
  // Constructors.
  BbrStateAdaptive();
  BbrStateAdaptive(TcpBbrAdaptive *owner);

  // Destructor.
  virtual ~BbrStateAdaptive();

  // Get state type.
  virtual bbra::bbr_state getType() const=0;

  // Invoked when state first entered.
  virtual void enter();

  // Invoked when state updated.
  virtual void execute()=0;

  // Invoked when state exited.
  virtual void exit();

 protected:
  TcpBbrAdaptive *m_owner;             // BBR' flow that owns state.
};

///////////////////////////////////////////////
// BBR' STARTUP state

class BbrStartupState : public BbrStateAdaptive {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrStartupState(TcpBbrAdaptive *owner);
  BbrStartupState();

  // Get state type.
  bbra::bbr_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  double m_full_bw;                        // Max prev BW in STARTUP.
  int m_full_bw_count;                     // Times BW not grown in STARTUP.
};

///////////////////////////////////////////////
// BBR' DRAIN

class BbrDrainState : public BbrStateAdaptive {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrDrainState(TcpBbrAdaptive *owner);
  BbrDrainState();

  // Get state type.
  bbra::bbr_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  uint32_t m_inflight_limit; // Target bytes in flight to exit DRAIN state.
  uint32_t m_round_count;    // Number of rounds in DRAIN state.
};

///////////////////////////////////////////////
// BBR' PROBE_BW

class BbrProbeBWState : public BbrStateAdaptive {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrProbeBWState(TcpBbrAdaptive *owner);
  BbrProbeBWState();

  // Get state type.
  bbra::bbr_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  int m_gain_cycle;                        // For cycling gain in PROBE_BW.
};

///////////////////////////////////////////////
// BBR' PROBE_RTT

class BbrProbeRTTState : public BbrStateAdaptive {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  BbrProbeRTTState(TcpBbrAdaptive *owner);
  BbrProbeRTTState();

  // Get state type.
  bbra::bbr_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  Time m_probe_rtt_time;     // Time to remain in PROBE_RTT.
};

} // end of namespace ns3
#endif