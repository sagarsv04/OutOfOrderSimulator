# Project 2 - Announcements


Handling JUMPs and BTB in Project 2
============

Posted on: Sunday, December 1, 2019 11:22:06 AM EST

JUMPs use the Branch Unit to calculate the target address.  When a branch is decoded, all following instructions are flushed till the JUMPs target is calculated, as specified.  The flushed instructions may include a subsequent JUMP.  Because of this, a BIS entry is not needed for a JUMP, as the direction of control flow from a JUMP is known.

The BTB is looked up in the Fetch stage, in parallel with instruction fetching.  BTB lookup for BZ and BNZ gives the prediction and target address and the next instruction fetched is from the predicted path.  A BIS entry is created in the D/RF stage.


Branch prediction Policy
============

Posted on: Wednesday, November 27, 2019 1:58:04 PM EST

The project handout does not specify the branch predictor to use. This is the predictor you should use:

- A fully-associative BTB is used, looked up by the address of the instruction. The BTB has 8 entries.

- A default prediction of NOT taken is used for BZ, BNZ if an entry does not exist in the BTB

- A branch is predicted to be taken if the last execution of the branch, as recorded in the BTB, was taken.

- A JUMP is always predicted as taken.

- Assume that BTB entries are not replaced for simplicity . This means we have up to 8 BZs or BNZs in any code.


Some Q and A on Project 2
============

Posted on: Friday, December 6, 2019 2:37:39 PM EST

Q: I have a question about the memory access instruction. Does a memory access instruction
need to wait until it becomes the head of the LSQ and ROB to perform the memory access operation?
- Answer: Yes.

Q: How do we release physical registers?
- Answer: Use the criteria described in the notes: 1) all consumers have read the register AND 2) the
physical register is no longer the most recent instance of the associated architectural register.  You can
use a consumer counter, as described earlier.
