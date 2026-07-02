#+private
package rio

import "core:flags"
import "core:fmt"
import "core:os"

Args :: struct {
	dir: string `args:"pos=0" usage:"Dir to build or run"`,
}

main :: proc() {
	args: Args
	flags.parse_or_exit(&args, os.args, .Unix)
	switch {
	case args.dir == "":
		fmt.println("TODO Show help")
	case:
		process_dir(args.dir)
	}
}
