/*************************************************************************/
/*  file_dialog.cpp                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "file_dialog.h"

#include "core/os/keyboard.h"
#include "core/string/print_string.h"
#include "scene/gui/label.h"

FileDialog::GetIconFunc FileDialog::get_icon_func = nullptr;
FileDialog::GetIconFunc FileDialog::get_large_icon_func = nullptr;

FileDialog::RegisterFunc FileDialog::register_func = nullptr;
FileDialog::RegisterFunc FileDialog::unregister_func = nullptr;

void FileDialog::popup_file_dialog() {
	popup_centered_clamped(Size2i(700, 500), 0.8f);
	_focus_file_text();
}

void FileDialog::_focus_file_text() {
	int lp = file->get_text().rfind(".");
	if (lp != -1) {
		file->select(0, lp);
		if (file->is_inside_tree() && !get_tree()->is_node_being_edited(file)) {
			file->grab_focus();
		}
	}
}

VBoxContainer *FileDialog::get_vbox() {
	return vbox;
}

void FileDialog::_theme_changed() {
	Color font_color = vbox->get_theme_color(SNAME("font_color"), SNAME("Button"));
	Color font_hover_color = vbox->get_theme_color(SNAME("font_hover_color"), SNAME("Button"));
	Color font_focus_color = vbox->get_theme_color(SNAME("font_focus_color"), SNAME("Button"));
	Color font_pressed_color = vbox->get_theme_color(SNAME("font_pressed_color"), SNAME("Button"));

	dir_up->add_theme_color_override("icon_normal_color", font_color);
	dir_up->add_theme_color_override("icon_hover_color", font_hover_color);
	dir_up->add_theme_color_override("icon_focus_color", font_focus_color);
	dir_up->add_theme_color_override("icon_pressed_color", font_pressed_color);

	dir_prev->add_theme_color_override("icon_color_normal", font_color);
	dir_prev->add_theme_color_override("icon_color_hover", font_hover_color);
	dir_prev->add_theme_color_override("icon_focus_color", font_focus_color);
	dir_prev->add_theme_color_override("icon_color_pressed", font_pressed_color);

	dir_next->add_theme_color_override("icon_color_normal", font_color);
	dir_next->add_theme_color_override("icon_color_hover", font_hover_color);
	dir_next->add_theme_color_override("icon_focus_color", font_focus_color);
	dir_next->add_theme_color_override("icon_color_pressed", font_pressed_color);

	refresh->add_theme_color_override("icon_normal_color", font_color);
	refresh->add_theme_color_override("icon_hover_color", font_hover_color);
	refresh->add_theme_color_override("icon_focus_color", font_focus_color);
	refresh->add_theme_color_override("icon_pressed_color", font_pressed_color);

	show_hidden->add_theme_color_override("icon_normal_color", font_color);
	show_hidden->add_theme_color_override("icon_hover_color", font_hover_color);
	show_hidden->add_theme_color_override("icon_focus_color", font_focus_color);
	show_hidden->add_theme_color_override("icon_pressed_color", font_pressed_color);
}

void FileDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!is_visible()) {
				set_process_shortcut_input(false);
			}
		} break;

		case NOTIFICATION_ENTER_TREE: {
			dir_up->set_icon(vbox->get_theme_icon(SNAME("parent_folder"), SNAME("FileDialog")));
			if (vbox->is_layout_rtl()) {
				dir_prev->set_icon(vbox->get_theme_icon(SNAME("forward_folder"), SNAME("FileDialog")));
				dir_next->set_icon(vbox->get_theme_icon(SNAME("back_folder"), SNAME("FileDialog")));
			} else {
				dir_prev->set_icon(vbox->get_theme_icon(SNAME("back_folder"), SNAME("FileDialog")));
				dir_next->set_icon(vbox->get_theme_icon(SNAME("forward_folder"), SNAME("FileDialog")));
			}
			refresh->set_icon(vbox->get_theme_icon(SNAME("reload"), SNAME("FileDialog")));
			show_hidden->set_icon(vbox->get_theme_icon(SNAME("toggle_hidden"), SNAME("FileDialog")));
			_theme_changed();
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED: {
			update_filters();
		} break;
	}
}

void FileDialog::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && has_focus()) {
		if (k->is_pressed()) {
			bool handled = true;

			switch (k->get_keycode()) {
				case Key::H: {
					if (k->is_command_pressed()) {
						set_show_hidden_files(!show_hidden_files);
					} else {
						handled = false;
					}

				} break;
				case Key::F5: {
					invalidate();
				} break;
				case Key::BACKSPACE: {
					_dir_submitted("..");
				} break;
				default: {
					handled = false;
				}
			}

			if (handled) {
				set_input_as_handled();
			}
		}
	}
}

void FileDialog::set_enable_multiple_selection(bool p_enable) {
	tree->set_select_mode(p_enable ? Tree::SELECT_MULTI : Tree::SELECT_SINGLE);
};

Vector<String> FileDialog::get_selected_files() const {
	Vector<String> list;

	TreeItem *item = tree->get_root();
	while ((item = tree->get_next_selected(item))) {
		list.push_back(dir_access->get_current_dir().path_join(item->get_text(0)));
	};

	return list;
};

void FileDialog::update_dir() {
	if (root_prefix.is_empty()) {
		dir->set_text(dir_access->get_current_dir(false));
	} else {
		dir->set_text(dir_access->get_current_dir(false).trim_prefix(root_prefix).trim_prefix("/"));
	}

	if (drives->is_visible()) {
		if (dir_access->get_current_dir().is_network_share_path()) {
			_update_drives(false);
			drives->add_item(RTR("Network"));
			drives->set_item_disabled(-1, true);
			drives->select(drives->get_item_count() - 1);
		} else {
			drives->select(dir_access->get_current_drive());
		}
	}

	// Deselect any item, to make "Select Current Folder" button text by default.
	deselect_all();
}

void FileDialog::_dir_submitted(String p_dir) {
	_change_dir(root_prefix.path_join(p_dir));
	file->set_text("");
	_push_history();
}

void FileDialog::_file_submitted(const String &p_file) {
	_action_pressed();
}

void FileDialog::_save_confirm_pressed() {
	String f = dir_access->get_current_dir().path_join(file->get_text());
	emit_signal(SNAME("file_selected"), f);
	hide();
}

void FileDialog::_post_popup() {
	ConfirmationDialog::_post_popup();
	if (invalidated) {
		update_file_list();
		invalidated = false;
	}
	if (mode == FILE_MODE_SAVE_FILE) {
		file->grab_focus();
	} else {
		tree->grab_focus();
	}

	set_process_shortcut_input(true);

	// For open dir mode, deselect all items on file dialog open.
	if (mode == FILE_MODE_OPEN_DIR) {
		deselect_all();
		file_box->set_visible(false);
	} else {
		file_box->set_visible(true);
	}

	local_history.clear();
	local_history_pos = -1;
	_push_history();
}

void FileDialog::_push_history() {
	local_history.resize(local_history_pos + 1);
	String new_path = dir_access->get_current_dir();
	if (local_history.size() == 0 || new_path != local_history[local_history_pos]) {
		local_history.push_back(new_path);
		local_history_pos++;
		dir_prev->set_disabled(local_history_pos == 0);
		dir_next->set_disabled(true);
	}
}

void FileDialog::_action_pressed() {
	if (mode == FILE_MODE_OPEN_FILES) {
		TreeItem *ti = tree->get_next_selected(nullptr);
		String fbase = dir_access->get_current_dir();

		Vector<String> files;
		while (ti) {
			files.push_back(fbase.path_join(ti->get_text(0)));
			ti = tree->get_next_selected(ti);
		}

		if (files.size()) {
			emit_signal(SNAME("files_selected"), files);
			hide();
		}

		return;
	}

	String file_text = file->get_text();
	String f = file_text.is_absolute_path() ? file_text : dir_access->get_current_dir().path_join(file_text);

	if ((mode == FILE_MODE_OPEN_ANY || mode == FILE_MODE_OPEN_FILE) && dir_access->file_exists(f)) {
		emit_signal(SNAME("file_selected"), f);
		hide();
	} else if (mode == FILE_MODE_OPEN_ANY || mode == FILE_MODE_OPEN_DIR) {
		String path = dir_access->get_current_dir();

		path = path.replace("\\", "/");
		TreeItem *item = tree->get_selected();
		if (item) {
			Dictionary d = item->get_metadata(0);
			if (d["dir"] && d["name"] != "..") {
				path = path.path_join(d["name"]);
			}
		}

		emit_signal(SNAME("dir_selected"), path);
		hide();
	}

	if (mode == FILE_MODE_SAVE_FILE) {
		bool valid = false;

		if (filter->get_selected() == filter->get_item_count() - 1) {
			valid = true; // match none
		} else if (filters.size() > 1 && filter->get_selected() == 0) {
			// match all filters
			for (int i = 0; i < filters.size(); i++) {
				String flt = filters[i].get_slice(";", 0);
				for (int j = 0; j < flt.get_slice_count(","); j++) {
					String str = flt.get_slice(",", j).strip_edges();
					if (f.match(str)) {
						valid = true;
						break;
					}
				}
				if (valid) {
					break;
				}
			}
		} else {
			int idx = filter->get_selected();
			if (filters.size() > 1) {
				idx--;
			}
			if (idx >= 0 && idx < filters.size()) {
				String flt = filters[idx].get_slice(";", 0);
				int filterSliceCount = flt.get_slice_count(",");
				for (int j = 0; j < filterSliceCount; j++) {
					String str = (flt.get_slice(",", j).strip_edges());
					if (f.match(str)) {
						valid = true;
						break;
					}
				}

				if (!valid && filterSliceCount > 0) {
					String str = (flt.get_slice(",", 0).strip_edges());
					f += str.substr(1, str.length() - 1);
					file->set_text(f.get_file());
					valid = true;
				}
			} else {
				valid = true;
			}
		}

		if (!valid) {
			exterr->popup_centered(Size2(250, 80));
			return;
		}

		if (dir_access->file_exists(f)) {
			confirm_save->set_text(RTR("File exists, overwrite?"));
			confirm_save->popup_centered(Size2(200, 80));
		} else {
			emit_signal(SNAME("file_selected"), f);
			hide();
		}
	}
}

void FileDialog::_cancel_pressed() {
	file->set_text("");
	invalidate();
	hide();
}

bool FileDialog::_is_open_should_be_disabled() {
	if (mode == FILE_MODE_OPEN_ANY || mode == FILE_MODE_SAVE_FILE) {
		return false;
	}

	TreeItem *ti = tree->get_next_selected(tree->get_root());
	while (ti) {
		TreeItem *prev_ti = ti;
		ti = tree->get_next_selected(tree->get_root());
		if (ti == prev_ti) {
			break;
		}
	}
	// We have something that we can't select?
	if (!ti) {
		return mode != FILE_MODE_OPEN_DIR; // In "Open folder" mode, having nothing selected picks the current folder.
	}

	Dictionary d = ti->get_metadata(0);

	// Opening a file, but selected a folder? Forbidden.
	return ((mode == FILE_MODE_OPEN_FILE || mode == FILE_MODE_OPEN_FILES) && d["dir"]) || // Flipped case, also forbidden.
			(mode == FILE_MODE_OPEN_DIR && !d["dir"]);
}

void FileDialog::_go_up() {
	_change_dir("..");
	_push_history();
}

void FileDialog::_go_back() {
	if (local_history_pos <= 0) {
		return;
	}

	local_history_pos--;
	_change_dir(local_history[local_history_pos]);

	dir_prev->set_disabled(local_history_pos == 0);
	dir_next->set_disabled(local_history_pos == local_history.size() - 1);
}

void FileDialog::_go_forward() {
	if (local_history_pos == local_history.size() - 1) {
		return;
	}

	local_history_pos++;
	_change_dir(local_history[local_history_pos]);

	dir_prev->set_disabled(local_history_pos == 0);
	dir_next->set_disabled(local_history_pos == local_history.size() - 1);
}

void FileDialog::deselect_all() {
	// Clear currently selected items in file manager.
	tree->deselect_all();

	// And change get_ok title.
	if (!tree->is_anything_selected()) {
		get_ok_button()->set_disabled(_is_open_should_be_disabled());

		switch (mode) {
			case FILE_MODE_OPEN_FILE:
			case FILE_MODE_OPEN_FILES:
				set_ok_button_text(RTR("Open"));
				break;
			case FILE_MODE_OPEN_DIR:
				set_ok_button_text(RTR("Select Current Folder"));
				break;
			case FILE_MODE_OPEN_ANY:
			case FILE_MODE_SAVE_FILE:
				// FIXME: Implement, or refactor to avoid duplication with set_mode
				break;
		}
	}
}

void FileDialog::_tree_multi_selected(Object *p_object, int p_cell, bool p_selected) {
	_tree_selected();
}

void FileDialog::_tree_selected() {
	TreeItem *ti = tree->get_selected();
	if (!ti) {
		return;
	}
	Dictionary d = ti->get_metadata(0);

	if (!d["dir"]) {
		file->set_text(d["name"]);
	} else if (mode == FILE_MODE_OPEN_DIR) {
		set_ok_button_text(RTR("Select This Folder"));
	}

	get_ok_button()->set_disabled(_is_open_should_be_disabled());
}

void FileDialog::_tree_item_activated() {
	TreeItem *ti = tree->get_selected();
	if (!ti) {
		return;
	}

	Dictionary d = ti->get_metadata(0);

	if (d["dir"]) {
		_change_dir(d["name"]);
		if (mode == FILE_MODE_OPEN_FILE || mode == FILE_MODE_OPEN_FILES || mode == FILE_MODE_OPEN_DIR || mode == FILE_MODE_OPEN_ANY) {
			file->set_text("");
		}
		_push_history();
	} else {
		_action_pressed();
	}
}

void FileDialog::update_file_name() {
	int idx = filter->get_selected() - 1;
	if ((idx == -1 && filter->get_item_count() == 2) || (filter->get_item_count() > 2 && idx >= 0 && idx < filter->get_item_count() - 2)) {
		if (idx == -1) {
			idx += 1;
		}
		String filter_str = filters[idx];
		String file_str = file->get_text();
		String base_name = file_str.get_basename();
		Vector<String> filter_substr = filter_str.split(";");
		if (filter_substr.size() >= 2) {
			file_str = base_name + "." + filter_substr[0].strip_edges().get_extension().to_lower();
		} else {
			file_str = base_name + "." + filter_str.strip_edges().get_extension().to_lower();
		}
		file->set_text(file_str);
	}
}

void FileDialog::update_file_list() {
	tree->clear();

	// Scroll back to the top after opening a directory
	tree->get_vscroll_bar()->set_value(0);

	dir_access->list_dir_begin();

	if (dir_access->is_readable(dir_access->get_current_dir().utf8().get_data())) {
		message->hide();
	} else {
		message->set_text(RTR("You don't have permission to access contents of this folder."));
		message->show();
	}

	TreeItem *root = tree->create_item();
	Ref<Texture2D> folder = vbox->get_theme_icon(SNAME("folder"), SNAME("FileDialog"));
	Ref<Texture2D> file_icon = vbox->get_theme_icon(SNAME("file"), SNAME("FileDialog"));
	const Color folder_color = vbox->get_theme_color(SNAME("folder_icon_modulate"), SNAME("FileDialog"));
	const Color file_color = vbox->get_theme_color(SNAME("file_icon_modulate"), SNAME("FileDialog"));
	List<String> files;
	List<String> dirs;

	bool is_hidden;
	String item = dir_access->get_next();

	while (!item.is_empty()) {
		if (item == "." || item == "..") {
			item = dir_access->get_next();
			continue;
		}

		is_hidden = dir_access->current_is_hidden();

		if (show_hidden_files || !is_hidden) {
			if (!dir_access->current_is_dir()) {
				files.push_back(item);
			} else {
				dirs.push_back(item);
			}
		}
		item = dir_access->get_next();
	}

	dirs.sort_custom<NaturalNoCaseComparator>();
	files.sort_custom<NaturalNoCaseComparator>();

	while (!dirs.is_empty()) {
		String &dir_name = dirs.front()->get();
		TreeItem *ti = tree->create_item(root);
		ti->set_text(0, dir_name);
		ti->set_icon(0, folder);
		ti->set_icon_modulate(0, folder_color);

		Dictionary d;
		d["name"] = dir_name;
		d["dir"] = true;

		ti->set_metadata(0, d);

		dirs.pop_front();
	}

	List<String> patterns;
	// build filter
	if (filter->get_selected() == filter->get_item_count() - 1) {
		// match all
	} else if (filters.size() > 1 && filter->get_selected() == 0) {
		// match all filters
		for (int i = 0; i < filters.size(); i++) {
			String f = filters[i].get_slice(";", 0);
			for (int j = 0; j < f.get_slice_count(","); j++) {
				patterns.push_back(f.get_slice(",", j).strip_edges());
			}
		}
	} else {
		int idx = filter->get_selected();
		if (filters.size() > 1) {
			idx--;
		}

		if (idx >= 0 && idx < filters.size()) {
			String f = filters[idx].get_slice(";", 0);
			for (int j = 0; j < f.get_slice_count(","); j++) {
				patterns.push_back(f.get_slice(",", j).strip_edges());
			}
		}
	}

	String base_dir = dir_access->get_current_dir();

	while (!files.is_empty()) {
		bool match = patterns.is_empty();
		String match_str;

		for (const String &E : patterns) {
			if (files.front()->get().matchn(E)) {
				match_str = E;
				match = true;
				break;
			}
		}

		if (match) {
			TreeItem *ti = tree->create_item(root);
			ti->set_text(0, files.front()->get());

			if (get_icon_func) {
				Ref<Texture2D> icon = get_icon_func(base_dir.path_join(files.front()->get()));
				ti->set_icon(0, icon);
			} else {
				ti->set_icon(0, file_icon);
			}
			ti->set_icon_modulate(0, file_color);

			if (mode == FILE_MODE_OPEN_DIR) {
				ti->set_custom_color(0, vbox->get_theme_color(SNAME("files_disabled"), SNAME("FileDialog")));
				ti->set_selectable(0, false);
			}
			Dictionary d;
			d["name"] = files.front()->get();
			d["dir"] = false;
			ti->set_metadata(0, d);

			if (file->get_text() == files.front()->get() || match_str == files.front()->get()) {
				ti->select(0);
			}
		}

		files.pop_front();
	}

	if (tree->get_root() && tree->get_root()->get_first_child() && tree->get_selected() == nullptr) {
		tree->get_root()->get_first_child()->select(0);
	}
}

void FileDialog::_filter_selected(int) {
	update_file_name();
	update_file_list();
}

void FileDialog::update_filters() {
	filter->clear();

	if (filters.size() > 1) {
		String all_filters;

		const int max_filters = 5;

		for (int i = 0; i < MIN(max_filters, filters.size()); i++) {
			String flt = filters[i].get_slice(";", 0).strip_edges();
			if (i > 0) {
				all_filters += ", ";
			}
			all_filters += flt;
		}

		if (max_filters < filters.size()) {
			all_filters += ", ...";
		}

		filter->add_item(RTR("All Recognized") + " (" + all_filters + ")");
	}
	for (int i = 0; i < filters.size(); i++) {
		String flt = filters[i].get_slice(";", 0).strip_edges();
		String desc = filters[i].get_slice(";", 1).strip_edges();
		if (desc.length()) {
			filter->add_item(String(tr(desc)) + " (" + flt + ")");
		} else {
			filter->add_item("(" + flt + ")");
		}
	}

	filter->add_item(RTR("All Files") + " (*)");
}

void FileDialog::clear_filters() {
	filters.clear();
	update_filters();
	invalidate();
}

void FileDialog::add_filter(const String &p_filter, const String &p_description) {
	ERR_FAIL_COND_MSG(p_filter.begins_with("."), "Filter must be \"filename.extension\", can't start with dot.");
	if (p_description.is_empty()) {
		filters.push_back(p_filter);
	} else {
		filters.push_back(vformat("%s ; %s", p_filter, p_description));
	}
	update_filters();
	invalidate();
}

void FileDialog::set_filters(const Vector<String> &p_filters) {
	if (filters == p_filters) {
		return;
	}
	filters = p_filters;
	update_filters();
	invalidate();
}

Vector<String> FileDialog::get_filters() const {
	return filters;
}

String FileDialog::get_current_dir() const {
	return dir->get_text();
}

String FileDialog::get_current_file() const {
	return file->get_text();
}

String FileDialog::get_current_path() const {
	return dir->get_text().path_join(file->get_text());
}

void FileDialog::set_current_dir(const String &p_dir) {
	_change_dir(p_dir);

	_push_history();
}

void FileDialog::set_current_file(const String &p_file) {
	if (file->get_text() == p_file) {
		return;
	}
	file->set_text(p_file);
	update_dir();
	invalidate();
	_focus_file_text();
}

void FileDialog::set_current_path(const String &p_path) {
	if (!p_path.size()) {
		return;
	}
	int pos = MAX(p_path.rfind("/"), p_path.rfind("\\"));
	if (pos == -1) {
		set_current_file(p_path);
	} else {
		String dir = p_path.substr(0, pos);
		String file = p_path.substr(pos + 1, p_path.length());
		set_current_dir(dir);
		set_current_file(file);
	}
}

void FileDialog::set_root_subfolder(const String &p_root) {
	root_subfolder = p_root;
	ERR_FAIL_COND_MSG(!dir_access->dir_exists(p_root), "root_subfolder must be an existing sub-directory.");

	local_history.clear();
	local_history_pos = -1;

	dir_access->change_dir(root_subfolder);
	if (root_subfolder.is_empty()) {
		root_prefix = "";
	} else {
		root_prefix = dir_access->get_current_dir();
	}
	invalidate();
	update_dir();
}

String FileDialog::get_root_subfolder() const {
	return root_subfolder;
}

void FileDialog::set_mode_overrides_title(bool p_override) {
	mode_overrides_title = p_override;
}

bool FileDialog::is_mode_overriding_title() const {
	return mode_overrides_title;
}

void FileDialog::set_file_mode(FileMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 5);
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	switch (mode) {
		case FILE_MODE_OPEN_FILE:
			set_ok_button_text(RTR("Open"));
			if (mode_overrides_title) {
				set_title(TTRC("Open a File"));
			}
			makedir->hide();
			break;
		case FILE_MODE_OPEN_FILES:
			set_ok_button_text(RTR("Open"));
			if (mode_overrides_title) {
				set_title(TTRC("Open File(s)"));
			}
			makedir->hide();
			break;
		case FILE_MODE_OPEN_DIR:
			set_ok_button_text(RTR("Select Current Folder"));
			if (mode_overrides_title) {
				set_title(TTRC("Open a Directory"));
			}
			makedir->show();
			break;
		case FILE_MODE_OPEN_ANY:
			set_ok_button_text(RTR("Open"));
			if (mode_overrides_title) {
				set_title(TTRC("Open a File or Directory"));
			}
			makedir->show();
			break;
		case FILE_MODE_SAVE_FILE:
			set_ok_button_text(RTR("Save"));
			if (mode_overrides_title) {
				set_title(TTRC("Save a File"));
			}
			makedir->show();
			break;
	}

	if (mode == FILE_MODE_OPEN_FILES) {
		tree->set_select_mode(Tree::SELECT_MULTI);
	} else {
		tree->set_select_mode(Tree::SELECT_SINGLE);
	}
}

FileDialog::FileMode FileDialog::get_file_mode() const {
	return mode;
}

void FileDialog::set_access(Access p_access) {
	ERR_FAIL_INDEX(p_access, 3);
	if (access == p_access) {
		return;
	}
	switch (p_access) {
		case ACCESS_FILESYSTEM: {
			dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		} break;
		case ACCESS_RESOURCES: {
			dir_access = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		} break;
		case ACCESS_USERDATA: {
			dir_access = DirAccess::create(DirAccess::ACCESS_USERDATA);
		} break;
	}
	access = p_access;
	root_prefix = "";
	root_subfolder = "";
	_update_drives();
	invalidate();
	update_filters();
	update_dir();
}

void FileDialog::invalidate() {
	if (is_visible()) {
		update_file_list();
		invalidated = false;
	} else {
		invalidated = true;
	}
}

FileDialog::Access FileDialog::get_access() const {
	return access;
}

void FileDialog::_make_dir_confirm() {
	Error err = dir_access->make_dir(makedirname->get_text().strip_edges());
	if (err == OK) {
		_change_dir(makedirname->get_text().strip_edges());
		update_filters();
		_push_history();
	} else {
		mkdirerr->popup_centered(Size2(250, 50));
	}
	makedirname->set_text(""); // reset label
}

void FileDialog::_make_dir() {
	makedialog->popup_centered(Size2(250, 80));
	makedirname->grab_focus();
}

void FileDialog::_select_drive(int p_idx) {
	String d = drives->get_item_text(p_idx);
	_change_dir(d);
	file->set_text("");
	_push_history();
}

void FileDialog::_change_dir(const String &p_new_dir) {
	if (root_prefix.is_empty()) {
		dir_access->change_dir(p_new_dir);
	} else {
		String old_dir = dir_access->get_current_dir();
		dir_access->change_dir(p_new_dir);
		if (!dir_access->get_current_dir(false).begins_with(root_prefix)) {
			dir_access->change_dir(old_dir);
			return;
		}
	}

	invalidate();
	update_dir();
}

void FileDialog::_update_drives(bool p_select) {
	int dc = dir_access->get_drive_count();
	if (dc == 0 || access != ACCESS_FILESYSTEM) {
		drives->hide();
	} else {
		drives->clear();
		Node *dp = drives->get_parent();
		if (dp) {
			dp->remove_child(drives);
		}
		dp = dir_access->drives_are_shortcuts() ? shortcuts_container : drives_container;
		dp->add_child(drives);
		drives->show();

		for (int i = 0; i < dir_access->get_drive_count(); i++) {
			drives->add_item(dir_access->get_drive(i));
		}

		if (p_select) {
			drives->select(dir_access->get_current_drive());
		}
	}
}

bool FileDialog::default_show_hidden_files = false;

void FileDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_cancel_pressed"), &FileDialog::_cancel_pressed);

	ClassDB::bind_method(D_METHOD("clear_filters"), &FileDialog::clear_filters);
	ClassDB::bind_method(D_METHOD("add_filter", "filter", "description"), &FileDialog::add_filter, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("set_filters", "filters"), &FileDialog::set_filters);
	ClassDB::bind_method(D_METHOD("get_filters"), &FileDialog::get_filters);
	ClassDB::bind_method(D_METHOD("get_current_dir"), &FileDialog::get_current_dir);
	ClassDB::bind_method(D_METHOD("get_current_file"), &FileDialog::get_current_file);
	ClassDB::bind_method(D_METHOD("get_current_path"), &FileDialog::get_current_path);
	ClassDB::bind_method(D_METHOD("set_current_dir", "dir"), &FileDialog::set_current_dir);
	ClassDB::bind_method(D_METHOD("set_current_file", "file"), &FileDialog::set_current_file);
	ClassDB::bind_method(D_METHOD("set_current_path", "path"), &FileDialog::set_current_path);
	ClassDB::bind_method(D_METHOD("set_mode_overrides_title", "override"), &FileDialog::set_mode_overrides_title);
	ClassDB::bind_method(D_METHOD("is_mode_overriding_title"), &FileDialog::is_mode_overriding_title);
	ClassDB::bind_method(D_METHOD("set_file_mode", "mode"), &FileDialog::set_file_mode);
	ClassDB::bind_method(D_METHOD("get_file_mode"), &FileDialog::get_file_mode);
	ClassDB::bind_method(D_METHOD("get_vbox"), &FileDialog::get_vbox);
	ClassDB::bind_method(D_METHOD("get_line_edit"), &FileDialog::get_line_edit);
	ClassDB::bind_method(D_METHOD("set_access", "access"), &FileDialog::set_access);
	ClassDB::bind_method(D_METHOD("get_access"), &FileDialog::get_access);
	ClassDB::bind_method(D_METHOD("set_root_subfolder", "dir"), &FileDialog::set_root_subfolder);
	ClassDB::bind_method(D_METHOD("get_root_subfolder"), &FileDialog::get_root_subfolder);
	ClassDB::bind_method(D_METHOD("set_show_hidden_files", "show"), &FileDialog::set_show_hidden_files);
	ClassDB::bind_method(D_METHOD("is_showing_hidden_files"), &FileDialog::is_showing_hidden_files);
	ClassDB::bind_method(D_METHOD("_update_file_name"), &FileDialog::update_file_name);
	ClassDB::bind_method(D_METHOD("_update_dir"), &FileDialog::update_dir);
	ClassDB::bind_method(D_METHOD("_update_file_list"), &FileDialog::update_file_list);
	ClassDB::bind_method(D_METHOD("deselect_all"), &FileDialog::deselect_all);

	ClassDB::bind_method(D_METHOD("invalidate"), &FileDialog::invalidate);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mode_overrides_title"), "set_mode_overrides_title", "is_mode_overriding_title");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "file_mode", PROPERTY_HINT_ENUM, "Open File,Open Files,Open Folder,Open Any,Save"), "set_file_mode", "get_file_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "access", PROPERTY_HINT_ENUM, "Resources,User Data,File System"), "set_access", "get_access");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "root_subfolder"), "set_root_subfolder", "get_root_subfolder");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "filters"), "set_filters", "get_filters");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_hidden_files"), "set_show_hidden_files", "is_showing_hidden_files");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "current_dir", PROPERTY_HINT_DIR, "", PROPERTY_USAGE_NONE), "set_current_dir", "get_current_dir");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "current_file", PROPERTY_HINT_FILE, "*", PROPERTY_USAGE_NONE), "set_current_file", "get_current_file");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "current_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_current_path", "get_current_path");

	ADD_SIGNAL(MethodInfo("file_selected", PropertyInfo(Variant::STRING, "path")));
	ADD_SIGNAL(MethodInfo("files_selected", PropertyInfo(Variant::PACKED_STRING_ARRAY, "paths")));
	ADD_SIGNAL(MethodInfo("dir_selected", PropertyInfo(Variant::STRING, "dir")));

	BIND_ENUM_CONSTANT(FILE_MODE_OPEN_FILE);
	BIND_ENUM_CONSTANT(FILE_MODE_OPEN_FILES);
	BIND_ENUM_CONSTANT(FILE_MODE_OPEN_DIR);
	BIND_ENUM_CONSTANT(FILE_MODE_OPEN_ANY);
	BIND_ENUM_CONSTANT(FILE_MODE_SAVE_FILE);

	BIND_ENUM_CONSTANT(ACCESS_RESOURCES);
	BIND_ENUM_CONSTANT(ACCESS_USERDATA);
	BIND_ENUM_CONSTANT(ACCESS_FILESYSTEM);
}

void FileDialog::set_show_hidden_files(bool p_show) {
	if (show_hidden_files == p_show) {
		return;
	}
	show_hidden_files = p_show;
	invalidate();
}

bool FileDialog::is_showing_hidden_files() const {
	return show_hidden_files;
}

void FileDialog::set_default_show_hidden_files(bool p_show) {
	default_show_hidden_files = p_show;
}

FileDialog::FileDialog() {
	show_hidden_files = default_show_hidden_files;

	vbox = memnew(VBoxContainer);
	add_child(vbox, false, INTERNAL_MODE_FRONT);
	vbox->connect("theme_changed", callable_mp(this, &FileDialog::_theme_changed));

	mode = FILE_MODE_SAVE_FILE;
	set_title(TTRC("Save a File"));

	HBoxContainer *hbc = memnew(HBoxContainer);

	dir_prev = memnew(Button);
	dir_prev->set_flat(true);
	dir_prev->set_tooltip_text(RTR("Go to previous folder."));
	dir_next = memnew(Button);
	dir_next->set_flat(true);
	dir_next->set_tooltip_text(RTR("Go to next folder."));
	dir_up = memnew(Button);
	dir_up->set_flat(true);
	dir_up->set_tooltip_text(RTR("Go to parent folder."));
	hbc->add_child(dir_prev);
	hbc->add_child(dir_next);
	hbc->add_child(dir_up);
	dir_prev->connect("pressed", callable_mp(this, &FileDialog::_go_back));
	dir_next->connect("pressed", callable_mp(this, &FileDialog::_go_forward));
	dir_up->connect("pressed", callable_mp(this, &FileDialog::_go_up));

	hbc->add_child(memnew(Label(RTR("Path:"))));

	drives_container = memnew(HBoxContainer);
	hbc->add_child(drives_container);

	drives = memnew(OptionButton);
	drives->connect("item_selected", callable_mp(this, &FileDialog::_select_drive));
	hbc->add_child(drives);

	dir = memnew(LineEdit);
	dir->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	hbc->add_child(dir);
	dir->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	refresh = memnew(Button);
	refresh->set_flat(true);
	refresh->set_tooltip_text(RTR("Refresh files."));
	refresh->connect("pressed", callable_mp(this, &FileDialog::update_file_list));
	hbc->add_child(refresh);

	show_hidden = memnew(Button);
	show_hidden->set_flat(true);
	show_hidden->set_toggle_mode(true);
	show_hidden->set_pressed(is_showing_hidden_files());
	show_hidden->set_tooltip_text(RTR("Toggle the visibility of hidden files."));
	show_hidden->connect("toggled", callable_mp(this, &FileDialog::set_show_hidden_files));
	hbc->add_child(show_hidden);

	shortcuts_container = memnew(HBoxContainer);
	hbc->add_child(shortcuts_container);

	makedir = memnew(Button);
	makedir->set_text(RTR("Create Folder"));
	makedir->connect("pressed", callable_mp(this, &FileDialog::_make_dir));
	hbc->add_child(makedir);
	vbox->add_child(hbc);

	tree = memnew(Tree);
	tree->set_hide_root(true);
	vbox->add_margin_child(RTR("Directories & Files:"), tree, true);

	message = memnew(Label);
	message->hide();
	message->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	message->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	message->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	tree->add_child(message);

	file_box = memnew(HBoxContainer);
	file_box->add_child(memnew(Label(RTR("File:"))));
	file = memnew(LineEdit);
	file->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	file->set_stretch_ratio(4);
	file->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	file_box->add_child(file);
	filter = memnew(OptionButton);
	filter->set_stretch_ratio(3);
	filter->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	filter->set_clip_text(true); // too many extensions overflows it
	file_box->add_child(filter);
	vbox->add_child(file_box);

	dir_access = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	_update_drives();

	connect("confirmed", callable_mp(this, &FileDialog::_action_pressed));
	tree->connect("multi_selected", callable_mp(this, &FileDialog::_tree_multi_selected), CONNECT_DEFERRED);
	tree->connect("cell_selected", callable_mp(this, &FileDialog::_tree_selected), CONNECT_DEFERRED);
	tree->connect("item_activated", callable_mp(this, &FileDialog::_tree_item_activated));
	tree->connect("nothing_selected", callable_mp(this, &FileDialog::deselect_all));
	dir->connect("text_submitted", callable_mp(this, &FileDialog::_dir_submitted));
	file->connect("text_submitted", callable_mp(this, &FileDialog::_file_submitted));
	filter->connect("item_selected", callable_mp(this, &FileDialog::_filter_selected));

	confirm_save = memnew(ConfirmationDialog);
	add_child(confirm_save, false, INTERNAL_MODE_FRONT);

	confirm_save->connect("confirmed", callable_mp(this, &FileDialog::_save_confirm_pressed));

	makedialog = memnew(ConfirmationDialog);
	makedialog->set_title(RTR("Create Folder"));
	VBoxContainer *makevb = memnew(VBoxContainer);
	makedialog->add_child(makevb);

	makedirname = memnew(LineEdit);
	makedirname->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	makevb->add_margin_child(RTR("Name:"), makedirname);
	add_child(makedialog, false, INTERNAL_MODE_FRONT);
	makedialog->register_text_enter(makedirname);
	makedialog->connect("confirmed", callable_mp(this, &FileDialog::_make_dir_confirm));
	mkdirerr = memnew(AcceptDialog);
	mkdirerr->set_text(RTR("Could not create folder."));
	add_child(mkdirerr, false, INTERNAL_MODE_FRONT);

	exterr = memnew(AcceptDialog);
	exterr->set_text(RTR("Must use a valid extension."));
	add_child(exterr, false, INTERNAL_MODE_FRONT);

	update_filters();
	update_dir();

	set_hide_on_ok(false);

	if (register_func) {
		register_func(this);
	}
}

FileDialog::~FileDialog() {
	if (unregister_func) {
		unregister_func(this);
	}
}
