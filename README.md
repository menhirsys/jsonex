jsonex - an allocations-free, streaming JSON parser/extractor
-

`jsonex` is a JSON parser with useful properties for embedded or
resource-constrained environments:
* it does not use `malloc` or the heap, and
* it can be fed one character at a time ("streaming"), rather than requiring
the entire JSON input at once.

`jsonex` parses JSON, but unlike other JSON parsers, it does not return a parse
tree. Instead, you tell it what information you want to extract from the JSON
structure, and then it finds that information for you.
