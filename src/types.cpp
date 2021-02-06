#include "types.hpp"


char current(SourceFile source) {
	return *(source.it);
}

bool at_end(SourceFile source) {
	if (source.it >= source.end) return true;
	else                         return false;
}

bool match(SourceFile source, std::string to_match) {
	for (size_t i = 0; i < to_match.size(); i++) {
		if (current(source) != to_match[i]) return false;
		source = source + 1;
	}
	return true;
}

SourceFile addOne(SourceFile source) {
	source.it++;
	if (current(source) == '\n') {
		source.line += 1;
		source.col = 1;
	}
	else {
		source.col++;
	}
	return source;
}

SourceFile operator+(SourceFile source, size_t offset) {
	if (offset == 0) return source;
	else             return addOne(source) + (offset - 1);
}
