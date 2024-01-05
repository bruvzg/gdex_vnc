#ifndef GDEX_VNC_TEXTURE_CLASS_H
#define GDEX_VNC_TEXTURE_CLASS_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
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

private:
	enum connection_state {
		VNC_NOT_CONNECTED,
		VNC_CONNECTING,
		VNC_CONNECTED,
		VNC_WILL_BE_DISCONNECTED
	};

	struct msg {
		int type; // 0 = mouse, 1 = keyboard
		union {
			int x;
			int scancode;
		};
		union {
			int y;
			bool is_down;
		};
		int mask;
	};

	int width;
	int height;
	int bits_per_pixel;
	char host[128];
	char password[128];
	connection_state connection_status;

	uint8_t *framebuffer;
	bool framebuffer_allocated;
	bool framebuffer_is_dirty;
	PackedByteArray image_data;

	std::thread * vnc_thread;
	std::mutex vnc_mutex;
	std::queue<msg> vnc_queue;

	float time_passed_since_update;
	float target_fps;

public:
	static rfbBool gdvnc_rfb_resize(rfbClient *p_client);
	static void gdvnc_rfb_update(rfbClient *p_client, int p_x, int p_y, int p_w, int p_h);
	static void gdvnc_rfb_got_cut_text(rfbClient *p_client, const char *p_text, int p_textlen);
	static void gdvnc_rfb_kbd_leds(rfbClient *p_client, int p_value, int p_pad);
	static void gdvnc_rfb_text_chat(rfbClient *p_client, int p_value, char *p_text);
	static char *gdvnc_rfb_get_password(rfbClient *p_client);
	static void vnc_main(GDEXVNC_Texture *p_texture);

	GDEXVNC_Texture();
	~GDEXVNC_Texture();

	void lock();
	void unlock();

	void set_target_fps(float p_fps);
	float get_target_fps();

	void connect(String p_host, String p_password);
	void disconnect();
	Vector2 get_screen_size();
	int status();
	bool update_texture(float p_delta);
	bool update_mouse_state(int p_x, int p_y, int p_mask);
	bool update_key_state(int p_scancode, bool p_is_down);
};

#endif // GDEX_VNC_TEXTURE_CLASS_H
