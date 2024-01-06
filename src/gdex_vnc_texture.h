#ifndef GDEX_VNC_TEXTURE_CLASS_H
#define GDEX_VNC_TEXTURE_CLASS_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <godot_cpp/core/binder_common.hpp>

#include <thread>
#include <mutex>
#include <queue> 

#include <rfb/rfbclient.h>

using namespace godot;

class GDEXVNC_Texture : public ImageTexture {
	GDCLASS(GDEXVNC_Texture, ImageTexture);

protected:
	static void _bind_methods();

public:
	enum VNCConnectionState {
		VNC_NOT_CONNECTED,
		VNC_CONNECTING,
		VNC_CONNECTED,
		VNC_WILL_BE_DISCONNECTED,
	};

private:
	enum VNCMessageType {
		VNC_MSG_NONE,
		VNC_MSG_KEY_EVENT,
		VNC_MSG_MOUSE_EVENT,
		VNC_MSG_CLIPBOARD_TEXT,
		VNC_MSG_CHAT_OPEN,
		VNC_MSG_CHAT_TEXT,
		VNC_MSG_CHAT_CLOSE,
		VNC_MSG_CHAT_FINISH,
		VNC_MSG_SET_DECKTOP_SIZE,
	};

	struct VNCMessage {
		VNCMessageType type = VNC_MSG_NONE;

		String text;
		uint32_t key;
		bool pressed;
		Vector2i position;
	};

	Vector2i size;
	CharString host_temp;
	CharString password_temp;
	String password;
	String host;
	VNCConnectionState connection_status = VNC_NOT_CONNECTED;

	bool framebuffer_dirty = false;
	bool framebuffer_resized = false;
	PackedByteArray framebuffer_data;

	std::thread *vnc_thread = nullptr;
	mutable std::mutex vnc_mutex;
	std::queue<VNCMessage> message_queue;

	float time_passed_since_update = 0.0;
	float target_fps = 30.0;

	String clipboard;

public:
	static rfbBool gdvnc_rfb_resize(rfbClient *p_client);
	static void gdvnc_rfb_update(rfbClient *p_client, int p_x, int p_y, int p_w, int p_h);
	static void gdvnc_rfb_got_cut_text(rfbClient *p_client, const char *p_text, int p_textlen);
	static void gdvnc_rfb_kbd_leds(rfbClient *p_client, int p_value, int p_pad);
	static void gdvnc_rfb_bell(rfbClient *p_client);
	static void gdvnc_rfb_text_chat(rfbClient *p_client, int p_value, char *p_text);
	static char *gdvnc_rfb_get_password(rfbClient *p_client);
	static void vnc_main(GDEXVNC_Texture *p_texture);

	static void init_key_mapping();

	void _emit_deferred(const String &p_name);
	void _emit_deferred_clipboard(const String &p_text);
	void _emit_deferred_leds(int p_value, int p_pad);
	void _emit_deferred_chat(const String &p_text);

	GDEXVNC_Texture();
	~GDEXVNC_Texture();

	void set_target_fps(float p_fps);
	float get_target_fps();

	void connect(const String &p_host, const String &p_password);
	void disconnect();
	VNCConnectionState status();
	bool update_texture(float p_delta);

	bool send_mouse_event(const Vector2i &p_position, int p_button_mask);
	bool send_key_event(Key p_keycode, bool p_pressed);

	Vector2i get_desktop_size();
	bool set_desktop_size(const Vector2i &p_size);

	String get_clipboard() const;
	bool set_clipboard(const String &p_text);
	bool chat_open();
	bool chat_close();
	bool chat_finish();
	bool chat_send(const String &p_text);
};

VARIANT_ENUM_CAST(GDEXVNC_Texture::VNCConnectionState);

#endif // GDEX_VNC_TEXTURE_CLASS_H
