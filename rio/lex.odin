#+private
package rio

Token :: struct {
	kind:   Token_Kind,
	text:   string, // Length and memory valid but value bogus for some token kinds.
	offset: int,
}

Token_Kind :: enum {
	Unknown,
	Const_Def,
	End,
	Line_End,
	Name,
	Round_Open,
	Round_Close,
	Space,
	String_Open,
	String_Text,
	String_Close,
	Tab,
}

lex :: proc(source: string) {
	offset := 0
}
