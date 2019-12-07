Project 2 - Announcements
============

Handling JUMPs and BTB in Project 2

Posted on: Sunday, December 1, 2019 11:22:06 AM EST

JUMPs use the Branch Unit to calculate the target address.  When a branch is decoded, all following instructions are flushed till the JUMPs target is calculated, as specified.  The flushed instructions may include a subsequent JUMP.  Because of this, a BIS entry is not needed for a JUMP, as the direction of control flow from a JUMP is known.

The BTB is looked up in the Fetch stage, in parallel with instruction fetching.  BTB lookup for BZ and BNZ gives the prediction and target address and the next instruction fetched is from the predicted path.  A BIS entry is created in the D/RF stage.
