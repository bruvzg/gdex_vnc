extends Node3D

var vnc_texture

func _conn():
	print("connected")

func _dconn():
	print("disconnected")

func _failed():
	print("failed")

# Called when the node enters the scene tree for the first time.
func _ready():
	vnc_texture = GDEXVNC_Texture.new()
	vnc_texture.connected.connect(_conn)
	vnc_texture.disconnected.connect(_dconn)
	vnc_texture.connection_failed.connect(_failed)

	vnc_texture.connect("localhost", "")
	$CSGMesh3D.material.albedo_texture = vnc_texture


func _input(event):
	if vnc_texture and event is InputEventKey:
			vnc_texture.send_key_event(event.physical_keycode, event.pressed)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if vnc_texture:
		vnc_texture.update_texture(delta)
