#include "gdex_vnc_texture.h"

#include <godot_cpp/core/memory.hpp>

using namespace godot;

#define RFBTAG (void *)0x123456712345678

void GDEXVNC_Texture::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_fps", "fps"), &GDEXVNC_Texture::set_target_fps);
	ClassDB::bind_method(D_METHOD("get_target_fps"), &GDEXVNC_Texture::get_target_fps);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "target_fps"), "set_target_fps", "get_target_fps");

	ClassDB::bind_method(D_METHOD("connect", "host", "password"), &GDEXVNC_Texture::connect);
	ClassDB::bind_method(D_METHOD("disconnect"), &GDEXVNC_Texture::disconnect);
	ClassDB::bind_method(D_METHOD("status"), &GDEXVNC_Texture::status);
	ClassDB::bind_method(D_METHOD("update_texture", "delta"), &GDEXVNC_Texture::update_texture);

	ClassDB::bind_method(D_METHOD("send_mouse_event", "position", "button_mask"), &GDEXVNC_Texture::send_mouse_event);
	ClassDB::bind_method(D_METHOD("send_key_event", "keycode", "pressed"), &GDEXVNC_Texture::send_key_event);

	ClassDB::bind_method(D_METHOD("get_desktop_size"), &GDEXVNC_Texture::get_desktop_size);
	ClassDB::bind_method(D_METHOD("set_desktop_size", "size"), &GDEXVNC_Texture::set_desktop_size);

	ClassDB::bind_method(D_METHOD("get_clipboard"), &GDEXVNC_Texture::get_clipboard);
	ClassDB::bind_method(D_METHOD("set_clipboard", "text"), &GDEXVNC_Texture::set_clipboard);

	ClassDB::bind_method(D_METHOD("chat_open"), &GDEXVNC_Texture::chat_open);
	ClassDB::bind_method(D_METHOD("chat_close"), &GDEXVNC_Texture::chat_close);
	ClassDB::bind_method(D_METHOD("chat_finish"), &GDEXVNC_Texture::chat_finish);
	ClassDB::bind_method(D_METHOD("chat_send", "text"), &GDEXVNC_Texture::chat_send);

	ClassDB::bind_method(D_METHOD("_emit_deferred", "name"), &GDEXVNC_Texture::_emit_deferred);
	ClassDB::bind_method(D_METHOD("_emit_deferred_clipboard", "text"), &GDEXVNC_Texture::_emit_deferred_clipboard);
	ClassDB::bind_method(D_METHOD("_emit_deferred_leds", "value", "pad"), &GDEXVNC_Texture::_emit_deferred_leds);
	ClassDB::bind_method(D_METHOD("_emit_deferred_chat", "text"), &GDEXVNC_Texture::_emit_deferred_chat);

	BIND_ENUM_CONSTANT(VNC_NOT_CONNECTED);
	BIND_ENUM_CONSTANT(VNC_CONNECTING);
	BIND_ENUM_CONSTANT(VNC_CONNECTED);
	BIND_ENUM_CONSTANT(VNC_WILL_BE_DISCONNECTED);

	ADD_SIGNAL(MethodInfo("bell"));
	ADD_SIGNAL(MethodInfo("clipboard_changed", PropertyInfo(Variant::STRING, "text")));
	ADD_SIGNAL(MethodInfo("keyboard_led_state_changed", PropertyInfo(Variant::INT, "value"), PropertyInfo(Variant::INT, "pad")));

	ADD_SIGNAL(MethodInfo("chat_opened"));
	ADD_SIGNAL(MethodInfo("chat_closed"));
	ADD_SIGNAL(MethodInfo("chat_finished"));
	ADD_SIGNAL(MethodInfo("chat_message", PropertyInfo(Variant::STRING, "text")));

	ADD_SIGNAL(MethodInfo("connected"));
	ADD_SIGNAL(MethodInfo("disconnected"));
	ADD_SIGNAL(MethodInfo("connection_failed"));
}

void GDEXVNC_Texture::_emit_deferred(const String &p_name) {
	emit_signal(p_name);
}

void GDEXVNC_Texture::_emit_deferred_clipboard(const String &p_text) {
	emit_signal("clipboard_changed", p_text);
}

void GDEXVNC_Texture::_emit_deferred_leds(int p_value, int p_pad) {
	emit_signal("keyboard_led_state_changed", p_value, p_pad);
}

void GDEXVNC_Texture::_emit_deferred_chat(const String &p_text) {
	emit_signal("chat_message", p_text);
}

rfbBool GDEXVNC_Texture::gdvnc_rfb_resize(rfbClient *p_client) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->vnc_mutex.lock();

	// Store new size.
	texture->size = Vector2i(p_client->width, p_client->height);

	// Resize and set framebuffer.
	texture->framebuffer_data.resize(texture->size.x * texture->size.y * 4);
	p_client->frameBuffer = texture->framebuffer_data.ptrw();

	texture->time_passed_since_update = 0.0;
	texture->framebuffer_dirty = true;
	texture->framebuffer_resized = true;

	texture->vnc_mutex.unlock();

	return TRUE;
}

void GDEXVNC_Texture::gdvnc_rfb_update(rfbClient *p_client, int p_x, int p_y, int p_w, int p_h) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->vnc_mutex.lock();
	texture->framebuffer_dirty = true;
	texture->vnc_mutex.unlock();
}

void GDEXVNC_Texture::gdvnc_rfb_got_cut_text(rfbClient *p_client, const char *p_text, int p_text_len) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->vnc_mutex.lock();
	texture->clipboard = String::utf8(p_text, p_text_len);
	texture->call_deferred("_emit_deferred_clipboard", texture->clipboard);
	texture->vnc_mutex.unlock();
}

void GDEXVNC_Texture::gdvnc_rfb_kbd_leds(rfbClient *p_client, int p_value, int p_pad) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);
	texture->vnc_mutex.lock();
	texture->call_deferred("_emit_deferred_leds", p_value, p_pad);
	texture->vnc_mutex.unlock();
}

void GDEXVNC_Texture::gdvnc_rfb_bell(rfbClient *p_client) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);
	texture->vnc_mutex.lock();
	texture->call_deferred("_emit_deferred", "bell");
	texture->vnc_mutex.unlock();
}

void GDEXVNC_Texture::gdvnc_rfb_text_chat(rfbClient *p_client, int p_value, char *p_text) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->vnc_mutex.lock();
	if (p_text == nullptr) {
		if (p_value == rfbTextChatOpen) {
			texture->call_deferred("_emit_deferred", "chat_opened");
		} else if (p_value == rfbTextChatClose) {
			texture->call_deferred("_emit_deferred", "chat_closed");
		} else if (p_value == rfbTextChatFinished) {
			texture->call_deferred("_emit_deferred", "chat_finished");
		}
	} else {
		texture->call_deferred("_emit_deferred_chat", String::utf8(p_text, p_value));
	}
	texture->vnc_mutex.unlock();
}

char *GDEXVNC_Texture::gdvnc_rfb_get_password(rfbClient *p_client) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->vnc_mutex.lock();
	texture->password_temp = texture->password.utf8();
	texture->vnc_mutex.unlock();

	return texture->password_temp.ptrw();
}

GDEXVNC_Texture::GDEXVNC_Texture() {
	// NOP
}

GDEXVNC_Texture::~GDEXVNC_Texture() {
	disconnect();
}

void GDEXVNC_Texture::set_target_fps(float p_fps) {
	if (p_fps < 0) {
		return;
	}

	target_fps = p_fps;
}

float GDEXVNC_Texture::get_target_fps() {
	return target_fps;
}

void GDEXVNC_Texture::vnc_main(GDEXVNC_Texture *p_texture) {
	p_texture->host_temp = p_texture->host.utf8();

	rfbClient *rfbCl = rfbGetClient(8, 3, 4);

	rfbCl->MallocFrameBuffer = GDEXVNC_Texture::gdvnc_rfb_resize;
	rfbCl->canHandleNewFBSize = TRUE;
	rfbCl->GotFrameBufferUpdate = GDEXVNC_Texture::gdvnc_rfb_update;
	rfbCl->GotXCutText = GDEXVNC_Texture::gdvnc_rfb_got_cut_text;
	rfbCl->GotXCutTextUTF8 = GDEXVNC_Texture::gdvnc_rfb_got_cut_text;
	rfbCl->HandleKeyboardLedState = GDEXVNC_Texture::gdvnc_rfb_kbd_leds;
	rfbCl->HandleTextChat = GDEXVNC_Texture::gdvnc_rfb_text_chat;
	rfbCl->GetPassword = GDEXVNC_Texture::gdvnc_rfb_get_password;
	rfbCl->Bell = GDEXVNC_Texture::gdvnc_rfb_bell;

	rfbClientSetClientData(rfbCl, RFBTAG, p_texture);

	char name[] = "godot_vnc";
	int argc = 2;
	char *argv[2] = { name, p_texture->host_temp.ptrw() };

	if (rfbInitClient(rfbCl, &argc, argv)) {
		p_texture->connection_status = VNC_CONNECTED;

		p_texture->call_deferred("_emit_deferred", "connected");

		while (p_texture->connection_status == VNC_CONNECTED) {
			int cnt = WaitForMessage(rfbCl, 1);
			if (cnt < 0) {
				p_texture->connection_status = VNC_NOT_CONNECTED;
			} else if (cnt > 0) {
				if (!HandleRFBServerMessage(rfbCl)) {
					p_texture->connection_status = VNC_NOT_CONNECTED;
				}
			}

			if (p_texture->connection_status == VNC_CONNECTED) {
				while (p_texture->message_queue.size() > 0) {
					VNCMessage message = p_texture->message_queue.front();
					p_texture->message_queue.pop();

					switch (message.type) {
						case VNC_MSG_KEY_EVENT: {
							SendKeyEvent(rfbCl, message.key, message.pressed);
						} break;
						case VNC_MSG_MOUSE_EVENT: {
							SendPointerEvent(rfbCl, message.position.x, message.position.y, message.key);
						} break;
						case VNC_MSG_CLIPBOARD_TEXT: {
							CharString cs = message.text.utf8();
							SendClientCutTextUTF8(rfbCl, cs.ptrw(), cs.length());
						} break;
						case VNC_MSG_CHAT_OPEN: {
							TextChatOpen(rfbCl);
						} break;
						case VNC_MSG_CHAT_TEXT: {
							CharString cs = message.text.utf8();
							TextChatSend(rfbCl, cs.ptrw());
						} break;
						case VNC_MSG_CHAT_CLOSE: {
							TextChatClose(rfbCl);
						} break;
						case VNC_MSG_CHAT_FINISH: {
							TextChatFinish(rfbCl);
						} break;
						case VNC_MSG_SET_DECKTOP_SIZE: {
							SendExtDesktopSize(rfbCl, message.position.x, message.position.y);
						} break;
						default:
							break;
					}
				}

				// yield, maybe enhance this to sleep the thread for a few ms
				std::this_thread::yield();
			}
		}
		rfbClientCleanup(rfbCl);

		p_texture->connection_status = VNC_NOT_CONNECTED;
		p_texture->call_deferred("_emit_deferred", "disconnected");
	} else {
		p_texture->call_deferred("_emit_deferred", "connection_failed");
	}
}

void GDEXVNC_Texture::connect(const String &p_host, const String &p_password) {
	disconnect();

	password = p_password;
	host = p_host;
	clipboard = String();

	connection_status = VNC_CONNECTING;

	vnc_thread = new std::thread(GDEXVNC_Texture::vnc_main, this);
}

void GDEXVNC_Texture::disconnect() {
	if (vnc_thread != nullptr) {
		// if we're still connected, make sure our loop exists...
		vnc_mutex.lock();
		connection_status = VNC_WILL_BE_DISCONNECTED;
		vnc_mutex.unlock();

		// join up with our thread if it hasn't already completed
		vnc_thread->join();

		// and cleanup our thread
		delete vnc_thread;
		vnc_thread = nullptr;
	}

	// should already be set but just in case...
	connection_status = VNC_NOT_CONNECTED;

	// finally make sure queue is empty
	while (message_queue.size() > 0) {
		message_queue.pop();
	}
}

String GDEXVNC_Texture::get_clipboard() const {
	String ret;

	vnc_mutex.lock();
	ret = clipboard;
	vnc_mutex.unlock();

	return ret;
}

Vector2i GDEXVNC_Texture::get_desktop_size() {
	Vector2i ret;

	vnc_mutex.lock();
	ret = size;
	vnc_mutex.unlock();

	return ret;
}

GDEXVNC_Texture::VNCConnectionState GDEXVNC_Texture::status() {
	VNCConnectionState ret;

	vnc_mutex.lock();
	ret = connection_status;
	vnc_mutex.unlock();

	return ret;
}

bool GDEXVNC_Texture::update_texture(float p_delta) {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED && framebuffer_dirty) {
		// Updates can update only small parts of the window, we build in a delay before we update our texture.
		time_passed_since_update += p_delta;
		if (time_passed_since_update > (1.0 / target_fps)) {
			Ref<Image> img = Image::create_from_data(size.x, size.y, false, Image::FORMAT_RGBA8, framebuffer_data);
			if (framebuffer_resized) {
				set_image(img);
			} else {
				update(img);
				framebuffer_resized = false;
			}

			framebuffer_dirty = false;
			time_passed_since_update = 0.0;
		}
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::send_mouse_event(const Vector2i &p_position, int p_mask) {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		uint32_t mask = 0;

		if ((p_mask & MouseButtonMask::MOUSE_BUTTON_MASK_LEFT) == MouseButtonMask::MOUSE_BUTTON_MASK_LEFT) {
			mask += rfbButton1Mask;
		}
		if ((p_mask & MouseButtonMask::MOUSE_BUTTON_MASK_RIGHT) == MouseButtonMask::MOUSE_BUTTON_MASK_RIGHT) {
			mask += rfbButton3Mask;
		}
		if ((p_mask & MouseButtonMask::MOUSE_BUTTON_MASK_MIDDLE) == MouseButtonMask::MOUSE_BUTTON_MASK_MIDDLE) {
			mask += rfbButton2Mask;
		}
		if ((p_mask & MouseButtonMask::MOUSE_BUTTON_MASK_MB_XBUTTON1) == MouseButtonMask::MOUSE_BUTTON_MASK_MB_XBUTTON1) {
			mask += rfbButton4Mask;
		}
		if ((p_mask & MouseButtonMask::MOUSE_BUTTON_MASK_MB_XBUTTON2) == MouseButtonMask::MOUSE_BUTTON_MASK_MB_XBUTTON2) {
			mask += rfbButton5Mask;
		}

		VNCMessage message;
		message.type = VNC_MSG_MOUSE_EVENT;
		message.position = p_position;
		message.key = mask;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

struct HashMapHasherKeys {
	static _FORCE_INLINE_ uint32_t hash(const Key p_key) { return hash_fmix32(static_cast<uint32_t>(p_key)); }
	static _FORCE_INLINE_ uint32_t hash(const uint32_t p_key) { return hash_fmix32(p_key); }
};

HashMap<Key, uint32_t, HashMapHasherKeys> key_map;

void GDEXVNC_Texture::init_key_mapping() {
	key_map[Key::KEY_ESCAPE] = XK_Escape;
	key_map[Key::KEY_TAB] = XK_Tab;
	key_map[Key::KEY_BACKSPACE] = XK_BackSpace;
	key_map[Key::KEY_ENTER] = XK_Return;
	key_map[Key::KEY_KP_ENTER] = XK_KP_Enter;
	key_map[Key::KEY_INSERT] = XK_Insert;
	key_map[Key::KEY_DELETE] = XK_Delete;
	key_map[Key::KEY_PAUSE] = XK_Pause;
	key_map[Key::KEY_PRINT] = XK_Print;
	key_map[Key::KEY_SYSREQ] = XK_Sys_Req;
	key_map[Key::KEY_CLEAR] = XK_Clear;
	key_map[Key::KEY_HOME] = XK_Home;
	key_map[Key::KEY_END] = XK_End;
	key_map[Key::KEY_LEFT] = XK_Left;
	key_map[Key::KEY_UP] = XK_Up;
	key_map[Key::KEY_RIGHT] = XK_Right;
	key_map[Key::KEY_DOWN] = XK_Down;
	key_map[Key::KEY_PAGEUP] = XK_Page_Up;
	key_map[Key::KEY_PAGEDOWN] = XK_Page_Down;
	key_map[Key::KEY_SHIFT] = XK_Shift_L;
	key_map[Key::KEY_CTRL] = XK_Control_L;
	key_map[Key::KEY_META] = XK_Super_L;
	key_map[Key::KEY_ALT] = XK_Alt_L;
	key_map[Key::KEY_CAPSLOCK] = XK_Caps_Lock;
	key_map[Key::KEY_NUMLOCK] = XK_Num_Lock;
	key_map[Key::KEY_SCROLLLOCK] = XK_Scroll_Lock;
	key_map[Key::KEY_F1] = XK_F1;
	key_map[Key::KEY_F2] = XK_F2;
	key_map[Key::KEY_F3] = XK_F3;
	key_map[Key::KEY_F4] = XK_F4;
	key_map[Key::KEY_F5] = XK_F5;
	key_map[Key::KEY_F6] = XK_F6;
	key_map[Key::KEY_F7] = XK_F7;
	key_map[Key::KEY_F8] = XK_F8;
	key_map[Key::KEY_F9] = XK_F9;
	key_map[Key::KEY_F10] = XK_F10;
	key_map[Key::KEY_F11] = XK_F11;
	key_map[Key::KEY_F12] = XK_F12;
	key_map[Key::KEY_F13] = XK_F13;
	key_map[Key::KEY_F14] = XK_F14;
	key_map[Key::KEY_F15] = XK_F15;
	key_map[Key::KEY_F16] = XK_F16;
	key_map[Key::KEY_F17] = XK_F17;
	key_map[Key::KEY_F18] = XK_F18;
	key_map[Key::KEY_F19] = XK_F19;
	key_map[Key::KEY_F20] = XK_F20;
	key_map[Key::KEY_F21] = XK_F21;
	key_map[Key::KEY_F22] = XK_F22;
	key_map[Key::KEY_F23] = XK_F23;
	key_map[Key::KEY_F24] = XK_F24;
	key_map[Key::KEY_F25] = XK_F25;
	key_map[Key::KEY_F26] = XK_F26;
	key_map[Key::KEY_F27] = XK_F27;
	key_map[Key::KEY_F28] = XK_F28;
	key_map[Key::KEY_F29] = XK_F29;
	key_map[Key::KEY_F30] = XK_F30;
	key_map[Key::KEY_F31] = XK_F31;
	key_map[Key::KEY_F32] = XK_F32;
	key_map[Key::KEY_F33] = XK_F33;
	key_map[Key::KEY_F34] = XK_F34;
	key_map[Key::KEY_F35] = XK_F35;
	key_map[Key::KEY_KP_MULTIPLY] = XK_KP_Multiply;
	key_map[Key::KEY_KP_DIVIDE] = XK_KP_Divide;
	key_map[Key::KEY_KP_SUBTRACT] = XK_KP_Subtract;
	key_map[Key::KEY_KP_PERIOD] = XK_KP_Decimal;
	key_map[Key::KEY_KP_ADD] = XK_KP_Add;
	key_map[Key::KEY_KP_0] = XK_KP_0;
	key_map[Key::KEY_KP_1] = XK_KP_1;
	key_map[Key::KEY_KP_2] = XK_KP_2;
	key_map[Key::KEY_KP_3] = XK_KP_3;
	key_map[Key::KEY_KP_4] = XK_KP_4;
	key_map[Key::KEY_KP_5] = XK_KP_5;
	key_map[Key::KEY_KP_6] = XK_KP_6;
	key_map[Key::KEY_KP_7] = XK_KP_7;
	key_map[Key::KEY_KP_8] = XK_KP_8;
	key_map[Key::KEY_KP_9] = XK_KP_9;
	key_map[Key::KEY_MENU] = XK_Menu;
	key_map[Key::KEY_HYPER] = XK_Hyper_L;
	key_map[Key::KEY_HELP] = XK_Help;
	key_map[Key::KEY_JIS_EISU] = XK_Eisu_Shift;
	key_map[Key::KEY_JIS_KANA] = XK_Kana_Shift;
	key_map[Key::KEY_SPACE] = XK_space;
	key_map[Key::KEY_EXCLAM] = XK_exclam;
	key_map[Key::KEY_QUOTEDBL] = XK_quotedbl;
	key_map[Key::KEY_NUMBERSIGN] = XK_numbersign;
	key_map[Key::KEY_DOLLAR] = XK_dollar;
	key_map[Key::KEY_PERCENT] = XK_percent;
	key_map[Key::KEY_AMPERSAND] = XK_ampersand;
	key_map[Key::KEY_APOSTROPHE] = XK_apostrophe;
	key_map[Key::KEY_PARENLEFT] = XK_parenleft;
	key_map[Key::KEY_PARENRIGHT] = XK_parenright;
	key_map[Key::KEY_ASTERISK] = XK_asterisk;
	key_map[Key::KEY_PLUS] = XK_plus;
	key_map[Key::KEY_COMMA] = XK_comma;
	key_map[Key::KEY_MINUS] = XK_minus;
	key_map[Key::KEY_PERIOD] = XK_period;
	key_map[Key::KEY_SLASH] = XK_slash;
	key_map[Key::KEY_0] = XK_0;
	key_map[Key::KEY_1] = XK_1;
	key_map[Key::KEY_2] = XK_2;
	key_map[Key::KEY_3] = XK_3;
	key_map[Key::KEY_4] = XK_4;
	key_map[Key::KEY_5] = XK_5;
	key_map[Key::KEY_6] = XK_6;
	key_map[Key::KEY_7] = XK_7;
	key_map[Key::KEY_8] = XK_8;
	key_map[Key::KEY_9] = XK_9;
	key_map[Key::KEY_COLON] = XK_colon;
	key_map[Key::KEY_SEMICOLON] = XK_semicolon;
	key_map[Key::KEY_LESS] = XK_less;
	key_map[Key::KEY_EQUAL] = XK_equal;
	key_map[Key::KEY_GREATER] = XK_greater;
	key_map[Key::KEY_QUESTION] = XK_question;
	key_map[Key::KEY_AT] = XK_at;
	key_map[Key::KEY_A] = XK_a;
	key_map[Key::KEY_B] = XK_b;
	key_map[Key::KEY_C] = XK_c;
	key_map[Key::KEY_D] = XK_d;
	key_map[Key::KEY_E] = XK_e;
	key_map[Key::KEY_F] = XK_f;
	key_map[Key::KEY_G] = XK_g;
	key_map[Key::KEY_H] = XK_h;
	key_map[Key::KEY_I] = XK_i;
	key_map[Key::KEY_J] = XK_j;
	key_map[Key::KEY_K] = XK_k;
	key_map[Key::KEY_L] = XK_l;
	key_map[Key::KEY_M] = XK_m;
	key_map[Key::KEY_N] = XK_n;
	key_map[Key::KEY_O] = XK_o;
	key_map[Key::KEY_P] = XK_p;
	key_map[Key::KEY_Q] = XK_q;
	key_map[Key::KEY_R] = XK_r;
	key_map[Key::KEY_S] = XK_s;
	key_map[Key::KEY_T] = XK_t;
	key_map[Key::KEY_U] = XK_u;
	key_map[Key::KEY_V] = XK_v;
	key_map[Key::KEY_W] = XK_w;
	key_map[Key::KEY_X] = XK_x;
	key_map[Key::KEY_Y] = XK_y;
	key_map[Key::KEY_Z] = XK_z;
	key_map[Key::KEY_BRACKETLEFT] = XK_bracketleft;
	key_map[Key::KEY_BACKSLASH] = XK_backslash;
	key_map[Key::KEY_BRACKETRIGHT] = XK_bracketright;
	key_map[Key::KEY_ASCIICIRCUM] = XK_asciicircum;
	key_map[Key::KEY_UNDERSCORE] = XK_underscore;
	key_map[Key::KEY_QUOTELEFT] = XK_quoteleft;
	key_map[Key::KEY_BRACELEFT] = XK_braceleft;
	key_map[Key::KEY_BAR] = XK_bar;
	key_map[Key::KEY_BRACERIGHT] = XK_braceright;
	key_map[Key::KEY_ASCIITILDE] = XK_asciitilde;
	key_map[Key::KEY_YEN] = XK_yen;
	key_map[Key::KEY_SECTION] = XK_section;
}

bool GDEXVNC_Texture::send_key_event(Key p_keycode, bool p_pressed) {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		const uint32_t *key = key_map.getptr(p_keycode);
		if (key) {
			VNCMessage message;
			message.type = VNC_MSG_KEY_EVENT;
			message.key = *key;
			message.pressed = p_pressed;

			message_queue.push(message);
		} else {
			success = false;	
		}
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::set_desktop_size(const Vector2i &p_size) {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		VNCMessage message;
		message.type = VNC_MSG_SET_DECKTOP_SIZE;
		message.position = p_size;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::set_clipboard(const String &p_text) {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		clipboard = p_text;

		VNCMessage message;
		message.type = VNC_MSG_CLIPBOARD_TEXT;
		message.text = p_text;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::chat_open() {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		VNCMessage message;
		message.type = VNC_MSG_CHAT_OPEN;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::chat_close() {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		VNCMessage message;
		message.type = VNC_MSG_CHAT_CLOSE;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::chat_finish() {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		VNCMessage message;
		message.type = VNC_MSG_CHAT_FINISH;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::chat_send(const String &p_text) {
	bool success = true;

	vnc_mutex.lock();
	if (connection_status == VNC_CONNECTED) {
		VNCMessage message;
		message.type = VNC_MSG_CHAT_TEXT;
		message.text = p_text;

		message_queue.push(message);
	} else {
		success = false;
	}
	vnc_mutex.unlock();

	return success;
}
