// djinterp
#include "test_session_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_session_run_transitions_and_noops
  Verifies the zero-argument run(): idle -> running, and a no-op from every
  other state.
  Tests the following:
  - run() from idle transitions to running
  - run() from running / paused / finished is a no-op (state unchanged)
*/
bool
tests_session_run_transitions_and_noops()
{
    session_type s;

    // idle -> running
    s.run();
    const bool to_running = s.is_running();

    // run() while running -> no-op
    s.run();
    const bool still_running = s.is_running();

    // running -> paused, then run() -> no-op
    s.pause();
    s.run();
    const bool still_paused = s.is_paused();

    // paused -> finished, then run() -> no-op
    s.finish();
    s.run();
    const bool still_finished = s.is_finished();

    return ( to_running    &&
             still_running &&
             still_paused  &&
             still_finished );
}

/*
tests_session_pause_transitions_and_noops
  Verifies pause(): running -> paused, and a no-op from every other state.
  Tests the following:
  - pause() from running transitions to paused and stops the timer
  - pause() from idle / paused / finished is a no-op
*/
bool
tests_session_pause_transitions_and_noops()
{
    // running -> paused (and timer stops)
    session_type s1;
    s1.run();
    s1.pause();
    const bool from_running =
        ( (s1.is_paused() == true) &&
          (s1.timer().running() == false) );

    // pause() from idle -> no-op
    session_type s2;
    s2.pause();
    const bool from_idle = s2.is_idle();

    // pause() from paused -> no-op (stays paused)
    session_type s3;
    s3.run();
    s3.pause();
    s3.pause();
    const bool from_paused = s3.is_paused();

    // pause() from finished -> no-op
    session_type s4;
    s4.run();
    s4.finish();
    s4.pause();
    const bool from_finished = s4.is_finished();

    return ( from_running &&
             from_idle    &&
             from_paused  &&
             from_finished );
}

/*
tests_session_resume_transitions_and_noops
  Verifies resume(): paused -> running, and a no-op from every other state.
  Tests the following:
  - resume() from paused transitions to running and restarts the timer
  - resume() from idle / running / finished is a no-op
*/
bool
tests_session_resume_transitions_and_noops()
{
    // paused -> running (and timer runs again)
    session_type s1;
    s1.run();
    s1.pause();
    s1.resume();
    const bool from_paused =
        ( (s1.is_running() == true) &&
          (s1.timer().running() == true) );

    // resume() from idle -> no-op
    session_type s2;
    s2.resume();
    const bool from_idle = s2.is_idle();

    // resume() from running -> no-op (stays running)
    session_type s3;
    s3.run();
    s3.resume();
    const bool from_running = s3.is_running();

    // resume() from finished -> no-op
    session_type s4;
    s4.run();
    s4.finish();
    s4.resume();
    const bool from_finished = s4.is_finished();

    return ( from_paused  &&
             from_idle    &&
             from_running &&
             from_finished );
}

/*
tests_session_finish_transitions_and_noops
  Verifies finish(): valid from running or paused, a no-op from idle or
  finished.  Exercises both operands of the compound guard.
  Tests the following:
  - finish() from running transitions to finished
  - finish() from paused transitions to finished
  - finish() from idle is a no-op
  - finish() from finished is a no-op
*/
bool
tests_session_finish_transitions_and_noops()
{
    // running -> finished
    session_type s1;
    s1.run();
    s1.finish();
    const bool from_running =
        ( (s1.is_finished() == true) &&
          (s1.timer().running() == false) );

    // paused -> finished
    session_type s2;
    s2.run();
    s2.pause();
    s2.finish();
    const bool from_paused = s2.is_finished();

    // idle -> no-op (stays idle)
    session_type s3;
    s3.finish();
    const bool from_idle = s3.is_idle();

    // finished -> no-op (stays finished)
    session_type s4;
    s4.run();
    s4.finish();
    s4.finish();
    const bool from_finished = s4.is_finished();

    return ( from_running &&
             from_paused  &&
             from_idle    &&
             from_finished );
}

/*
tests_session_reset_clears_state
  Verifies reset() returns the session to idle and clears the tree, counters,
  and timer - from any prior state.
  Tests the following:
  - reset() from finished returns to idle and clears everything
  - reset() from running also returns to idle and clears everything
*/
bool
tests_session_reset_clears_state()
{
    // reset from finished
    session_type s1;
    s1.tree().add_root(make_test(0, true));
    s1.passed().increment(5);
    s1.run();        // idle -> running (timer starts)
    s1.finish();     // -> finished (timer stops)
    s1.reset();

    const bool from_finished =
        ( (s1.is_idle()        == true) &&
          (s1.tree().size()    == 0)    &&
          (s1.passed().value() == 0)    &&
          (s1.failed().value() == 0)    &&
          (s1.timer().running() == false) &&
          (s1.elapsed().count() == 0) );

    // reset from running
    session_type s2;
    s2.tree().add_root(make_test(0, true));
    s2.errors().increment(3);
    s2.run();        // idle -> running
    s2.reset();

    const bool from_running =
        ( (s2.is_idle()        == true) &&
          (s2.tree().size()    == 0)    &&
          (s2.errors().value() == 0)    &&
          (s2.timer().running() == false) );

    return ( from_finished &&
             from_running );
}

/*
tests_session_lifecycle_sequence
  Verifies a full lifecycle walk through every state in order.
  Tests the following:
  - idle -> running -> paused -> running -> finished -> idle
*/
bool
tests_session_lifecycle_sequence()
{
    session_type s;

    const bool s0 = s.is_idle();

    s.run();
    const bool s1 = s.is_running();

    s.pause();
    const bool s2 = s.is_paused();

    s.resume();
    const bool s3 = s.is_running();

    s.finish();
    const bool s4 = s.is_finished();

    s.reset();
    const bool s5 = s.is_idle();

    return ( s0 && s1 && s2 && s3 && s4 && s5 );
}

/*
tests_session_timer_tracks_state
  Verifies the owned timer's running flag follows the lifecycle: it runs while
  running, stops while paused/finished, and is cleared by reset.
  Tests the following:
  - run() starts the timer; pause() stops it; resume() restarts it
  - finish() stops it; reset() clears it (and zeroes elapsed)
*/
bool
tests_session_timer_tracks_state()
{
    session_type s;

    const bool idle_stopped = (s.timer().running() == false);

    s.run();
    const bool run_started = (s.timer().running() == true);

    s.pause();
    const bool pause_stopped = (s.timer().running() == false);

    s.resume();
    const bool resume_started = (s.timer().running() == true);

    s.finish();
    const bool finish_stopped = (s.timer().running() == false);

    s.reset();
    const bool reset_cleared =
        ( (s.timer().running() == false) &&
          (s.elapsed().count() == 0) );

    return ( idle_stopped   &&
             run_started    &&
             pause_stopped  &&
             resume_started &&
             finish_stopped &&
             reset_cleared );
}


NS_END  // testing
NS_END  // djinterp
