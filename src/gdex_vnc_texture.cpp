#include "gdex_vnc_texture.h"

#include <godot_cpp/core/memory.hpp>

using namespace godot;

#define RFBTAG (void *)0x123456712345678

void GDEXVNC_Texture::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_target_fps", "fps"), &GDEXVNC_Texture::set_target_fps);
	ClassDB::bind_method(D_METHOD("get_target_fps"), &GDEXVNC_Texture::get_target_fps);

	ClassDB::bind_method(D_METHOD("connect", "host", "password"), &GDEXVNC_Texture::connect);
	ClassDB::bind_method(D_METHOD("disconnect"), &GDEXVNC_Texture::disconnect);
	ClassDB::bind_method(D_METHOD("get_screen_size"), &GDEXVNC_Texture::get_screen_size);
	ClassDB::bind_method(D_METHOD("status"), &GDEXVNC_Texture::status);
	ClassDB::bind_method(D_METHOD("update_texture", "delta"), &GDEXVNC_Texture::update_texture);
	ClassDB::bind_method(D_METHOD("update_mouse_state", "x", "y", "mask"), &GDEXVNC_Texture::update_mouse_state);
	ClassDB::bind_method(D_METHOD("update_key_state", "scancode", "is_down"), &GDEXVNC_Texture::update_key_state);
}

// rfb calls this whenever there is a resize of our screen. 
rfbBool GDEXVNC_Texture::gdvnc_rfb_resize(rfbClient *p_client) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->lock();

	if (texture->framebuffer_allocated) {
		// free our old
		memfree(texture->framebuffer);
		texture->framebuffer_allocated = false;
	}

	// we store our new size
	texture->width = p_client->width;
	texture->height = p_client->height;
	texture->bits_per_pixel = p_client->format.bitsPerPixel;

	printf("Resize received %i, %i (%i)\n", texture->width, texture->height, texture->bits_per_pixel);

	// and we allocate a new buffer
	texture->framebuffer = (uint8_t *)memalloc(texture->width * texture->height * 4);
	texture->framebuffer_allocated = (texture->framebuffer != NULL);

	if (texture->framebuffer_allocated) {
		texture->image_data.resize(texture->width * texture->height * 4);
	} else {
		printf("Failed to create framebuffer\n");
	}

	// tell our client about our frame buffer...
	p_client->frameBuffer = texture->framebuffer;

	// updating our texture happens after we receive updats
	texture->time_passed_since_update = 0.0;
	texture->framebuffer_is_dirty = true;

	texture->unlock();

	return TRUE;
}

// rfb calls this whenever a portion of the screen has to be updated
void GDEXVNC_Texture::gdvnc_rfb_update (rfbClient *p_client, int p_x, int p_y, int p_w, int p_h) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->lock();

	// all we do is set a flag that our framebuffer is dirty.
	texture->framebuffer_is_dirty = true;

	// updating our texture happens later

	texture->unlock();
}

// rfb calls this when data was copied onto the clipboard on the server
void GDEXVNC_Texture::gdvnc_rfb_got_cut_text(rfbClient *p_client, const char *p_text, int p_textlen) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// ignore for now but may copy this into our user data
}

// rfb calls this when our keyboard leds need to be turned on/off (numlock/caps/etc)
void GDEXVNC_Texture::gdvnc_rfb_kbd_leds(rfbClient *p_client, int p_value, int p_pad) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// ignore this
}

// rfb calls this when we have an action associated with our text chat
void GDEXVNC_Texture::gdvnc_rfb_text_chat(rfbClient *p_client, int p_value, char *p_text) {
	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// ignore this
}

// rfb calls this when the server requests a password
char *GDEXVNC_Texture::gdvnc_rfb_get_password(rfbClient *p_client) {
	char *pwd = NULL;

	GDEXVNC_Texture *texture = (GDEXVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	texture->lock();

	// libvnc is a bit too security concious. It will clear the memory and then free it
	// so yes.. we malloc, they free....
	pwd = (char *) calloc(1, strlen(texture->password)+1);
	strcpy(pwd, texture->password);

	texture->unlock();

	return pwd;
}

GDEXVNC_Texture::GDEXVNC_Texture() {
	vnc_thread = NULL;
	width = 0;
	height = 0;
	connection_status = VNC_NOT_CONNECTED;
	framebuffer_allocated = false;
	target_fps = 15.0;
	strcpy(password, "password");
}

GDEXVNC_Texture::~GDEXVNC_Texture() {
	printf("Cleanup gdvnc\n");

	// if we're still connected, disconnect...
	disconnect();
}

void GDEXVNC_Texture::lock() {
	vnc_mutex.lock();
}

void GDEXVNC_Texture::unlock() {
	vnc_mutex.unlock();
}

void GDEXVNC_Texture::set_target_fps(float p_fps) {
	if (p_fps < 0)
		return;

	target_fps = p_fps;
}

float GDEXVNC_Texture::get_target_fps() {
	return target_fps;
}

void GDEXVNC_Texture::vnc_main(GDEXVNC_Texture *p_texture) {
	char program[] = "godot";

	// create our client
	rfbClient *rfbCl = rfbGetClient(8, 3, 4);

	// set some callback pointers
	rfbCl->MallocFrameBuffer = GDEXVNC_Texture::gdvnc_rfb_resize;
	rfbCl->canHandleNewFBSize = TRUE;
	rfbCl->GotFrameBufferUpdate = GDEXVNC_Texture::gdvnc_rfb_update;
	rfbCl->GotXCutText = GDEXVNC_Texture::gdvnc_rfb_got_cut_text;
	rfbCl->HandleKeyboardLedState = GDEXVNC_Texture::gdvnc_rfb_kbd_leds;
	rfbCl->HandleTextChat = GDEXVNC_Texture::gdvnc_rfb_text_chat;
	rfbCl->GetPassword = GDEXVNC_Texture::gdvnc_rfb_get_password;

	// this lets rfb know that we want a pointer to our instance when it calls us
	rfbClientSetClientData(rfbCl, RFBTAG, p_texture);

	// initialize our client
	int argc = 2;
	char *argv[2];
	argv[0] = program;
	argv[1] = p_texture->host;

	if (rfbInitClient(rfbCl, &argc, argv)) {
		p_texture->connection_status = VNC_CONNECTED;
		printf("Success\n");

		// start our loop...
		while (p_texture->connection_status == VNC_CONNECTED) {
			// we want to be none blocking, even 1ms might be too long a timeout
			// should also check for negative result of waitformessage
			int cnt = WaitForMessage(rfbCl, 1);
			if (cnt < 0) {
				// need to log error and disconnect if we've lost connection
				p_texture->connection_status = VNC_NOT_CONNECTED;
			} else if (cnt > 0) {
				// check for server messages
				if (!HandleRFBServerMessage(rfbCl)) {
					// need to log error and disconnect if we've lost connection
					p_texture->connection_status = VNC_NOT_CONNECTED;						
				}
			}

			// if we're still connected, process client messages queue
			if (p_texture->connection_status == VNC_CONNECTED) {
				while (p_texture->vnc_queue.size() > 0) {
					msg message = p_texture->vnc_queue.front();
					p_texture->vnc_queue.pop();

					if (message.type == 0) {
						SendPointerEvent(rfbCl, message.x, message.y, message.mask);
					} else if (message.type == 1) {
						SendKeyEvent(rfbCl, message.scancode, message.is_down);
					}
				}

				// yield, maybe enhance this to sleep the thread for a few ms
				std::this_thread::yield();
			}
		}

		printf("Disconnected\n");

		rfbClientCleanup(rfbCl);
		p_texture->connection_status = VNC_NOT_CONNECTED;
	} else {
		printf("Failure\n");
	}

	if (p_texture->framebuffer_allocated) {
		// free our old
		memfree(p_texture->framebuffer);
		p_texture->framebuffer_allocated = false;
	}	
}

void GDEXVNC_Texture::connect(String p_host, String p_password) {
	// if we're already connected, lets disconnect
	disconnect();

	// we know our thread is not running so we are free to do stuff

	CharString hostAsUTF8 = p_host.utf8();
	strcpy(host, hostAsUTF8.get_data());

	CharString passwordAsUTF8 = p_password.utf8();
	strcpy(password, passwordAsUTF8.get_data());

	printf("Attempting connect to %s\n", host);
	connection_status = VNC_CONNECTING;

	vnc_thread = new std::thread(GDEXVNC_Texture::vnc_main, this);
}

void GDEXVNC_Texture::disconnect() {
	if (vnc_thread != NULL) {
		// if we're still connected, make sure our loop exists...
		vnc_mutex.lock();
		connection_status = VNC_WILL_BE_DISCONNECTED;
		vnc_mutex.unlock();

		// join up with our thread if it hasn't already completed
		vnc_thread->join();

		// and cleanup our thread
		delete vnc_thread;
		vnc_thread = NULL;
	}

	// should already be set but just in case...
	connection_status = VNC_NOT_CONNECTED;

	// finally make sure queue is empty
	while (vnc_queue.size() > 0) {
		vnc_queue.pop();
	}
}

Vector2 GDEXVNC_Texture::get_screen_size() {
	Vector2 ret;

	vnc_mutex.lock();

	ret.x=width;
	ret.y=height;

	vnc_mutex.unlock();

	return ret;
}

int GDEXVNC_Texture::status() {
	int ret;

	vnc_mutex.lock();

	ret = connection_status;

	vnc_mutex.unlock();

	return ret;
}

bool GDEXVNC_Texture::update_texture(float p_delta) {
	bool success = true;

	vnc_mutex.lock();

	if (connection_status == VNC_CONNECTED && framebuffer_allocated && framebuffer_is_dirty) {
		// updates can update only small parts of the window, we build in a delay before we update our texture
		time_passed_since_update += p_delta;
		if (time_passed_since_update > (1.0 / target_fps)) {
			// update screen
			if (framebuffer != NULL) {
				memcpy(image_data.ptrw(), framebuffer, width * height * 4);
			}

			// create an image from this
			Ref<Image> img = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, image_data);
			img->convert(Image::FORMAT_RGB8);
			set_image(img);

			framebuffer_is_dirty = false;
			time_passed_since_update = 0.0;
		}
	} else {
		// need to log error
		success = false;
	}

	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::update_mouse_state(int p_x, int p_y, int p_mask) {
	bool success = true;

	vnc_mutex.lock();

	if (connection_status == VNC_CONNECTED) {
		int mask = 0;

		// middle and right buttons are reversed
		if ((p_mask & 1) == 1) {
			mask += 1;
		}

		if ((p_mask & 2) == 2) {
			mask += 4;
		}

		if ((p_mask & 4) == 4) {
			mask += 2;
		}

		// add the other mouse buttons as is...
		mask += (p_mask & 0xFFF8);

		// add to queue
		msg message;
		message.type = 0;
		message.x = p_x;
		message.y = p_y;
		message.mask = p_mask;

		vnc_queue.push(message);
	} else {
		// need to log error
		success = false;
	}

	vnc_mutex.unlock();

	return success;
}

bool GDEXVNC_Texture::update_key_state(int p_scancode, bool p_is_down) {
	bool success = true;

	vnc_mutex.lock();

	if (connection_status == VNC_CONNECTED) {
		// VNC requires certain codes to be different
		switch (p_scancode) {
			case Key::KEY_ENTER:
				p_scancode = XK_Return;
				break;
			case Key::KEY_META:
				p_scancode = XK_Meta_L;
				break;
			case Key::KEY_BACKSPACE:
				p_scancode = XK_BackSpace; /* back space, back char */
				break;
			case Key::KEY_TAB:
				p_scancode = XK_Tab;
				break;
//			case Key::KEY_LF:
//				p_scancode = XK_Linefeed; /* Linefeed, LF */
//				break;
			case Key::KEY_CLEAR:
				p_scancode = XK_Clear;
				break;
			case Key::KEY_PAUSE:
				p_scancode = XK_Pause; /* Pause, hold */
				break;
			case Key::KEY_SCROLLLOCK:
				p_scancode = XK_Scroll_Lock;
				break;
//			case Key::KEY_SEQ_REQ:
//				p_scancode = XK_Sys_Req;
//				break;
			case Key::KEY_ESCAPE:
				p_scancode = XK_Escape;
				break;
			case Key::KEY_DELETE:
				p_scancode = XK_Delete; /* Delete, rubout */
				break;

			case Key::KEY_HOME:
				p_scancode = XK_Home;
				break;
			case Key::KEY_LEFT:
				p_scancode = XK_Left; /* Move left, left arrow */
				break;
			case Key::KEY_UP:
				p_scancode = XK_Up; /* Move up, up arrow */
				break;
			case Key::KEY_RIGHT:
				p_scancode = XK_Right; /* Move right, right arrow */
				break;
			case Key::KEY_DOWN:
				p_scancode = XK_Down; /* Move down, down arrow */
				break;
//			case Key::KEY_PRIOR:
//				p_scancode = XK_Prior; /* Prior, previous */
//				break;
			case Key::KEY_PAGEUP:
				p_scancode = XK_Page_Up;
				break;
//			case Key::KEY_NEXT:
//				p_scancode = XK_Next; /* Next */
//				break;
			case Key::KEY_PAGEDOWN:
				p_scancode = XK_Page_Down;
				break;
			case Key::KEY_END:
				p_scancode = XK_End; /* EOL */
				break;
//			case Key::KEY_BEGIN:
//				p_scancode = XK_Begin; /* BOL */
//				break;


			case Key::KEY_SHIFT:
				p_scancode = XK_Shift_L; /* Left shift */
				break;
//			case Key::KEY_SHIFT:
//				p_scancode = XK_Shift_R; /* Right shift */
//				break;
			case Key::KEY_CTRL:
				p_scancode = XK_Control_L; /* Left control */
				break;
//			case Key::KEY_CTRL:
//				p_scancode = XK_Control_R; /* Right control */
//				break;
			case Key::KEY_CAPSLOCK:
				p_scancode = XK_Caps_Lock; /* Caps lock */
				break;
//			case Key::KEY_SHIFTLOCK:
//				p_scancode = XK_Shift_Lock; /* Shift lock */
//				break;

//			case Key::KEY_META_L:
//				p_scancode = XK_Meta_L; /* Left meta */
//				break;
//			case Key::KEY_META_R:
//				p_scancode = XK_Meta_R; /* Right meta */
//				break;
			case Key::KEY_ALT:
				p_scancode = XK_Alt_L; /* Left alt */
				break;
//			case Key::KEY_ALT:
//				p_scancode = XK_Alt_R; /* Right alt */
//				break;

			default:
				// use as is
				break;
		}

		// add to queue
		msg message;
		message.type = 1;
		message.scancode = p_scancode;
		message.is_down = p_is_down;

		vnc_queue.push(message);
	} else {
		// need to log error
		success = false;
	}

	vnc_mutex.unlock();

	return success;
}
