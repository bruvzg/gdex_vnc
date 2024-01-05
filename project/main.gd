extends Node3D

var vnc_texture

# Called when the node enters the scene tree for the first time.
func _ready():
	vnc_texture = GDEXVNC_Texture.new()
	vnc_texture.connect("localhost", "")
	vnc_texture.set_target_fps(30.0)
	$Sprite3D.texture = vnc_texture


func _input(event):
	if vnc_texture and event is InputEventKey:
			vnc_texture.update_key_state(event.physical_keycode, event.pressed)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if vnc_texture:
		vnc_texture.update_texture(delta)
