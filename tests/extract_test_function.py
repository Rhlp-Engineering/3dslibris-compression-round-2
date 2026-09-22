"""Extract a production function for a platform-stubbed host regression test."""
import pathlib
import sys
text = pathlib.Path(sys.argv[1]).read_text()
start = text.index(sys.argv[2])
brace = text.index("{", start)
depth = 1
end = brace + 1
while depth:
    depth += (text[end] == "{") - (text[end] == "}")
    end += 1
print(text[start:end])
