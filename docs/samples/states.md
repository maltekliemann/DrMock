<!--
Copyright 2019 Ole Kliemann, Malte Kliemann

This file is part of DrMock.

DrMock is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

DrMock is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with DrMock.  If not, see <https://www.gnu.org/licenses/>.
-->

# samples/states

This sample demonstrates **DrMock**'s state calculus and how to use it
for state verification.

# Table of contents

* [Introduction](#introduction)
* [State calculus](#state-calculus)
* [Testing states](#testing-states)
* [Running the tests](#running-the-tests)
* [Configuring state behavior](#configuring-state-behavior)
  + [transition](#transition)
  + [returns](#returns)
  + [throws](#throws)
  + [polymorphic](#polymorphic)
* [State verification](#state-verification)
* [Bibliography](#bibliography)

### Project structure

```
samples/states
│   CMakeLists.txt
│   Makefile
│
└───src
│   │   CMakeLists.txt
│   │   IRocket.h
│   │   LaunchPad.cpp
│   │   LaunchPad.h
│
└───tests
    │   CMakeLists.txt
    │   LaunchPad.cpp
```

### Requirements

This project requires an installation of **DrMock** in `prefix/` or the
`CMAKE_PREFIX_PATH` environment variable. If your installation of
**DrMock** is located elsewhere, you must change the value of
`CMAKE_PREFIX_PATH`.

## Introduction

Mocks are usually used to test objects against their implementation. For
example, in [samples/mock](#samplesmock), expecting the behavior
```cpp
warehouse->mock.remove().push()
    .expect("foo", 2)
    .times(1)
    .returns(true);
```
only makes sense if `remove` is used in the implementation of `Order`.
This type of testing is called _behavior verification_ and is dependent
on the implementation of the system under test [1].

To make tests less dependent on the implementation, **DrMock**'s state
calculus may be used. Consider the interface `IRocket`:
```cpp
// IRocket.h

class IRocket
{
public:
  virtual ~IRocket() = default;

  virtual void toggleLeftThruster(bool) = 0;
  virtual void toggleRightThruster(bool) = 0;
  virtual void launch() = 0;
};
```
A rocket may be launched if at least one of its thrusters is toggled.
The `LaunchPad` is responsible for enabling at least one thruster before
launching the rocket:
```cpp
// LaunchPad.h

class LaunchPad
{
public:
  LaunchPad(std::shared_ptr<IRocket>);

  void launch();

private:
  std::shared_ptr<IRocket> rocket_;
};
```
Thus, one would expect the implementation of `LaunchPad::launch()` to
be:
```cpp
void
LaunchPad::launch()
{
  rocket_->toggleLeftThruster(true);
  rocket_->launch();
}
```

But let's say the control room is full of frantic apes randomly bashing
the buttons, before (luckily!) enabling a thruster and then pressing
`rocket->launch()`:
```cpp
void
LaunchPad::launch()
{
  // Randomly toggle the thrusters.
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::bernoulli_distribution dist{0.5};
  for (std::size_t i = 0; i < 19; i++)
  {
    rocket_->toggleLeftThruster(dist(gen));
    rocket_->toggleRightThruster(dist(gen));
  }

  // Toggle at least one thruster and engage!
  rocket_->toggleLeftThruster(true);
  rocket_->launch();
}
```
There is no way to predict the behavior of `LaunchPad::launch()`. Yet,
the result should be testable. This is where **DrMock**'s state calculus
enters the stage!

## State calculus

Every mock object admits a private `StateObject`, which manages an
arbitrary number of _slots_, each of which has a _current state_. This
state object is shared between all methods of the mock object, but, per
default, it is not used. To enable a method `func` to use the shared
state object, do
```cpp
foo->mock.func().state()
```
This call returns an instance of `StateBehavior`, which can be
configured in the same fashion as `Behavior`.

Every slot of the `StateObject` is designated using an `std::string`, as
is every state. Upon execution of the test, the state of every slot is
the _default state_ `""`.

The primary method of controlling the state object is by defining
_transitions_. To add a transition to a method, do
```cpp
rocket->mock.launch().state().transition(
    "main",
    "leftThrusterOn",
    "liftOff"
  );
```
This informs the state object to transition the slot `"main"` from the
state `"leftThrusterOn"` to `"liftOff"` when `launch()` is called.
If no slot is specified, as in
```cpp
rocket->mock.launch().state().transition(
    "leftThrusterOn",
    "liftOff"
  );
```
then the _default slot_ `""` is used.

**Note.** There is no need to add slots to the state object prior to
calling `transition`. This is done automatically.

Now, `launch` doesn't take any arguments. If the underlying methods
takes arguments, the `transition` call requires an input. For example, 
```cpp
rocket->mock.toggleLeftThruster().state().transition(
    "leftThrusterOn",
    "",
    false
  );
```
instructs the state object to transition the default slot from the state
`"leftThrusterOn"` to the default state `""` if
`toggleLeftThruster(false)` is called.

The wildcard symbol `"*"` may be used as catch-all/fallthrough symbol
for the current state. Pushing regular transitions before or after a
transition with wilcard add exceptions to the catch-all:
```cpp
rocket->mock.launch().state()
    .transition("*", "liftOff")
    .transition("", "failure");
```
If `launch()` is called, the default slots transitions to `"liftOff"`
from any state except the default state, which transitions to
`"failure"`.

## Testing states

As usual, the mock's behavior is configured at the start of the test:
Liftoff can only succeed if at least one thruster is on.
```cpp
  auto rocket = std::make_shared<drmock::samples::RocketMock>();

  // Define rocket's state behavior.
  rocket->mock.toggleLeftThruster().state()
      .transition("", "leftThrusterOn", true)
      .transition("leftThrusterOn", "", false)
      .transition("rightThrusterOn", "allThrustersOn", true)
      .transition("allThrustersOn", "rightThrusterOn", false);
  rocket->mock.toggleRightThruster().state()
      .transition("", "rightThrusterOn", true)
      .transition("rightThrusterOn", "", false)
      .transition("leftThrusterOn", "allThrustersOn", true)
      .transition("allThrustersOn", "leftThrusterOn", false);
  rocket->mock.launch().state()
      .transition("", "failure")
      .transition("*", "liftOff");
```
Recall that the state of every new slot is the default state `""`,
which, in this example, is used to model the `"allThrustersOff"` state.

After `launch()` is executed, the correctness of `launch()` is tested by
asserting that the current state of the default slot of `rocket` is
equal to `liftOff`:
```cpp
DRTEST_ASSERT(rocket->mock.verifyState("liftOff");
```
(The method
`bool verifyState([const std::string& slot,] const std::string& state);`
simply checks if the current state of `slot` is `state`.)

Thus, except for the configuration calls and the singular call to
`verifyState`, no access or knowledge of the implementation was required
to test `LaunchPad::launch()`. As demonstrated in
[Using DrMock for state verification](#using-drmock-for-state-verification),
one can sometimes even do better.

## Running the tests

Running the test should produce:
```
    Start 1: LaunchPadTest
1/1 Test #1: LaunchPadTest ....................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.01 sec
```
Feel free to rerun this test until you're convinced that the random
numbers generated in the implementation of `LaunchPad::launch()` have no
effect.

## Configuring state behavior

With the exception of `polymorphic`, the configuration methods take the
slot as optional first parameter. The default value is the default slot
`""`.

The parameters `Result` and `Args...` are designators for the underlying
method's return value and parameters.

Using the wildcard state `"*"` for the `current_state` parameter results
in the configuration serving as catch-all (or fallthru), to which other
configuration calls act as exceptions (as described above).

**Note.** The rules from [samples/mock](mock.md) regarding the
combination of configuration calls apply here. In other words, `returns`
and `emits` may be combined (and everything may be combined with
multiple calls to `transition`, of course), and an error if raised if
any of the configuration calls below are used to create double binds.
For example:
```cpp
bhv.returns("state1", 123)
   .emits("state1", &Dummy::f, 456)  // Ok, returns and emits are parallel.
   .throws("state2", std::runtime_error{""})  // Ok, different state.
   .throws("state1", std::logic_error{""})  // Fails, overriding previous state1 behavior.
   .returns("state2", 456)  // Ok, different slot.
   .returns("state2", 789)  // Not ok, raises error.
```

Furthermore, creating conflicting transition tables will result in an
error. For example:
```cpp
bhv.transition("state1", "state2", 2)
   .transition("state1", "state3", 3)  // Ok, different input.
   .transition("state1", "state4", 2)  // Not ok, will raise.
```

### transition

```cpp
StateBehavior& transition(
   [const std::string& slot,]
    std::string current_state,
    std::string new_state,
    Args... input
  );
```
Instruct the `StateBehavior` to transition the slot `slot` from the state
`current_state` to `new_state` when the method is called with the
arguments `input...`.

### returns

```cpp
StateBehavior& returns(
   [const std::string& slot,]
    const std::string& state,
    const Result& value
  );
StateBehavior& returns(
   [const std::string& slot,]
    const std::string& state,
    Result&& value
  );
```
Instruct the `StateBehavior` to return `value` if the state of `slot` is
`state`. `"*"` may not be used as catch-all in the `returns` method.

Example:
```cpp
// ILever.h

class ILever
{
public:
  virtual void set(bool) = 0;
  virtual bool get() = 0;
};

// Some test...

DRTEST_TEST(...)
{
  auto lever = std::shared_ptr<LeverMock>();
  lever->mock.toggle().state()
      .transition("", "on", true)
      .transition("on", "", false);
  lever->mock.get().state()
      .returns("", false)
      .returns("on", true);
}
```

### emits

```cpp
template<typename... SigArgs> StateBehavior& emits(
   [const std::string& state,]
    const std::string& slot,
    void (Class::*signal)(SigArgs...),
    SigArgs&&... args
  );
```
Instruct the `StateBehavior` to emit the Qt signal `signal` with
arguments `args` if the state of `slot` is `state` before returning.
Using `emits` outside of a Qt context (in other words, if
`DRMOCK_USE_QT` is not set) will raise an error when the method is
called.

### throws

```cpp
template<typename E> StateBehavior& throws(
   [const std::string& slot,]
    const std::string& state,
    E&& exception
  )
```
Instruct the `StateBehavior` to throw `exception` if the state of `slot`
is `state`.

### polymorphic

```cpp
template<typename... Deriveds> StateBehavior& polymorphic();
```
Instruct the `StateBehavior` to expect as argument
`std::shared_ptr<Deriveds>...` or `std::unique_ptr<Deriveds>...`.

See also: [Polymorphism](#polymorphism).

## State verification

Access to the mock object (except during configuration) can be entirely
eliminated in many cases, thus freeing the programmer to have any
knowledge of the implementation of the system under test.

Consider the following example: 
```cpp
class ILever
{
public:
  virtual void set(bool) = 0;
  virtual bool get() = 0;
};

class TrapDoor
{
public:
  TrapDoor(std::shared_ptr<ILever>);

  bool open()
  {
    return lever_.get();
  }
  void toggle(bool v)
  {
    lever_.set(v);
  }

private:
  std::shared_ptr<ILever> lever_;
};
```

Usually, verifying correctness of `TrapDoor` would look something like
this:
```cpp
DRTEST_TEST(toggle)
{
  // Configure mock.
  auto lever = std::make_shared<LeverMock>();
  lever->mock.toggle().expect(true);

  // Configure SUT.
  TrapDoor trap_door{lever};

  // Run the test.
  trap_door.toggle(true);
  DRTEST_VERIFY_MOCK(lever->mock);
}
```
In other words: Call `trap_door.toggle(...)` and verify that `lever`
behaves as expected. This is good old behavior verification.

But you can also do this:
```cpp
DRTEST_TEST(interactionOpenToggle)
{
  // Configure mock.
  auto lever = std::make_shared<LeverMock>();
  lever->mock.toggle().state()
      .transition("", "on", true)
      .transition("on", "", false);
  lever->mock.get().state()
      .returns("", false)
      .returns("on", true);

  // Configure SUT.
  TrapDoor trap_door{lever};

  // Run the test.
  trap_door.toggle(true);
  DRTEST_ASSERT(trap_door.open());
}
```
Note that although the lever's behavior was configured prior to the
test, it was not verified after calling `toggle`. Instead of testing the
behavior of each method of `TrapDoor` using mocks, the _interaction_
between `TrapDoor`'s methods is tested (the door is `open()` after
`toggle(true)`, etc.). Only the trap door's _state_ is verified using
`DRTEST_ASSERT(trap_door.open());`. This requires no knowledge of the
implementation, only the interface and the specified behavior. This is
essentially what's called _state verification_, and in this context
`lever` would be refered to as a _stub_, not a mock.

For more on mocks and stubs, see [1].

## Bibliography

[1] [M. Fowler, _Mocks Aren't Stubs_](https://martinfowler.com/articles/mocksArentStubs.html)
