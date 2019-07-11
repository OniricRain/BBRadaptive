//
// bbraState.h
//
// State machine (and states) for Tcpbbra.
//

#ifndef bbra_STATE_H
#define bbra_STATE_H

#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

class Tcpbbra; 
class bbraState;

namespace bbra {

// Defined bbra' states.
enum bbra_state {
  UNDEFINED_STATE=-1,
  STARTUP_STATE,
  DRAIN_STATE,
  PROBE_BW_STATE,
  PROBE_RTT_STATE,
};

} // end of namespace bbra

///////////////////////////////////////////////
// bbra' State Machine
class bbraStateMachine : public Object {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;

  // Constructor.
  bbraStateMachine();
  bbraStateMachine(Tcpbbra *owner);

  // Change state machine to new state.
  void changeState(bbraState *p_new_state);

  // Get type of current state.
  bbra::bbra_state getStateType() const;

  // Update by executing current state.
  void update();

 private:
  bbraState *m_state;           // Current state.
  Tcpbbra *m_owner;             // bbra' flow that owns machine.
};

///////////////////////////////////////////////
// bbra' State.
class bbraState : public Object {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  virtual std::string GetName() const;
  
  // Constructors.
  bbraState();
  bbraState(Tcpbbra *owner);

  // Destructor.
  virtual ~bbraState();

  // Get state type.
  virtual bbra::bbra_state getType() const=0;

  // Invoked when state first entered.
  virtual void enter();

  // Invoked when state updated.
  virtual void execute()=0;

  // Invoked when state exited.
  virtual void exit();

 protected:
  Tcpbbra *m_owner;             // bbra' flow that owns state.
};

///////////////////////////////////////////////
// bbra' STARTUP state

class bbraStartupState : public bbraState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  bbraStartupState(Tcpbbra *owner);
  bbraStartupState();

  // Get state type.
  bbra::bbra_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  double m_full_bw;                        // Max prev BW in STARTUP.
  int m_full_bw_count;                     // Times BW not grown in STARTUP.
};

///////////////////////////////////////////////
// bbra' DRAIN

class bbraDrainState : public bbraState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  bbraDrainState(Tcpbbra *owner);
  bbraDrainState();

  // Get state type.
  bbra::bbra_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  uint32_t m_inflight_limit; // Target bytes in flight to exit DRAIN state.
  uint32_t m_round_count;    // Number of rounds in DRAIN state.
};

///////////////////////////////////////////////
// bbra' PROBE_BW

class bbraProbeBWState : public bbraState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  bbraProbeBWState(Tcpbbra *owner);
  bbraProbeBWState();

  // Get state type.
  bbra::bbra_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  int m_gain_cycle;                        // For cycling gain in PROBE_BW.
};

///////////////////////////////////////////////
// bbra' PROBE_RTT

class bbraProbeRTTState : public bbraState {

 public:
  // Get type id.
  static TypeId GetTypeId();

  // Get name of object.
  std::string GetName() const;
  
  // Constructors.
  bbraProbeRTTState(Tcpbbra *owner);
  bbraProbeRTTState();

  // Get state type.
  bbra::bbra_state getType() const;

  // Invoked when state first entered.
  void enter();

  // Invoked when state updated.
  void execute();

 private:
  Time m_probe_rtt_time;     // Time to remain in PROBE_RTT.
};

} // end of namespace ns3
#endif
