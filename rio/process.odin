#+private
package rio

import "core:fmt"
import "core:log"
import "core:os"

process_dir :: proc(path: string) -> (err: os.Error) {
	// Read dir.
	file_infos: []os.File_Info
	if file_infos, err = os.read_all_directory_by_path(path, context.allocator); err != nil {
		log.errorf("Can't read directory: %s", path)
		return
	}
	defer os.file_info_slice_delete(file_infos, context.allocator)
	// Process files.
	for file_info in file_infos {
		// fmt.println(file_info)
		process_file(file_info.fullpath) or_return
	}
	return
}

process_file :: proc(path: string) -> (err: os.Error) {
	source := string(os.read_entire_file_from_path(path, context.allocator) or_return)
	lex(source)
	return
}
