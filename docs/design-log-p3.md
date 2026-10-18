# Project 3 Design Log
**Author:** <Anirudh Bhaskar> · **P2 code used:** <my own>
**Tools used:** <Claude for helping debug my WSL and git setup>

## 1. Hash table
For the ToolRegistry, I used separate chaining where each bucket will be a std::vector<Entry>. Entry will hold the tool's name and a std::unique_ptr<Tool>. The reason for chaining instead of open addressing is because it's simpler since I don't need to deal with things like probing the past full slots or considering if the table is completely full. Inserting and finding are just hashing the key, going to that bucket, loop through, done.

The hash function uses FNV-1a as stated in Appendix A. It is order sensitive and uses every character of the string. This is different from if we did something like s[0] + s.size() because it would send tool_1 to tool_9999 to a few buckets only.

At first, I started with 16 buckets and then resized when the load factor went over 0.75. I made sure to check this before inserting so that the load factor never gets above 1.0.

Measurements I got after inserting tool_0 to tool_9999:

1. bucket_count: 16384
2. load_factor: 0.610352
3. max_probe_length: 6

Additionally, I checked what s[0] + s.size() would do with the same names at the same bucket count, and got that the longest chain is 9000. This is every single name going into one of a few buckets, while mine spread everything out to make sure the worst bucket had 6 entries.

add() is O(1) because a resize is O(n), but only happens approximately every n/2 inserts since the last resize. If that O(n) cost is spread out over those cheap inserts, then each insert only pays a constant amount extra on average.

## 2. Pointer stability
A Tool* handed out by find() survives a resize because I stored the std::unique_ptr<Tool> inside each Entry. Each Entry is moved into the new bucket array during the resize. Since we have unique_ptr, only the pointer value is moved. The actual Tool object is safe and isn't touched, so even if an entry's bucket index changes after a resize, the address of the Tool in memory stays the same.

## 3. Calculator trace
1. See `2`, push onto values. values: [2]
2. See `*`, ops is empty so just push it. ops: [*]
3. See `(`, push it. ops: [*, (]
4. See `3`, push onto values. values: [2, 3]
5. See `+`, top of ops is `(` so just push, don't apply anything yet. ops: [*, (, +]
6. See `4`, push onto values. values: [2, 3, 4]
7. See `)`, so apply operators until we hit the `(`. The top is `+`, so apply it: pop 4 (right), pop 3 (left), compute 3+4=7, push 7. values: [2, 7]. Now top of ops is `(`, so pop that off too. ops: [*]
8. See `-`, the top of ops is `*` which has precedence 2, >= the precedence of `-` (which is 1), so apply `*` first: pop 7 (right), pop 2 (left), compute 2*7=14, push 14. values: [14]. ops is now empty, so push `-`. ops: [-]
9. See `5`, push onto values. values: [14, 5]
10. End of tokens, apply everything left on ops. Apply `-`: pop 5 (right), pop 14 (left), compute 14-5=9, push 9. values: [9]

Final answer: 9

## 4. Errors as values
In an agent loop, a tool failing is a normal outcome the model is expected to handle because a file might not exist or you might be dividing by 0. Since the model reads whatever comes back and decides what to do next, errors need to come back as actual data it can read (a ToolResult with ok=false and an error string), as opposed to a C++ exception that would unwind the stack and potentially crash the whole harness. Exceptions are for real bugs in the program itself. They aren't for things like if the user typed a bad file path. My resolve and calc_evaluate both follow this same pattern in that they return false and set an error message instead of throwing, since "path doesn't exist" or "divide by zero" are expected.
## 5. What I would design differently
One thing that I'd do differently is start the registry smaller because in the common case where only the 4 built in tools get registered, 16 buckets wastes space. I don't think it matters at the scale we are working with, but I think if this was part of a real system, it would matter more.