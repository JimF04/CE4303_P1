extends MeshInstance3D

var material: ShaderMaterial
var noise: Image

var noise_scale: float
var wave_speed: float
var height_scale: float

var time: float

func _ready():
	material = get_active_material(0) as ShaderMaterial
	
	if material:
		var tex = material.get_shader_parameter("wave")
		
		# 1. Check if the texture exists and is a NoiseTexture2D
		if tex is NoiseTexture2D:
			# 2. Wait for the noise to finish generating (important!)
			if tex.get_image() == null:
				await tex.changed
			
			# 3. Access the noise resource to call get_seamless_image
			if tex.noise:
				noise = tex.noise.get_seamless_image(512, 512)
			else:
				push_error("NoiseTexture2D does not have a Noise resource assigned!")
		
		# Grab your other parameters
		noise_scale = material.get_shader_parameter("noise_scale")
		wave_speed = material.get_shader_parameter("wave_speed")
		height_scale = material.get_shader_parameter("height_scale")
	
func _process(delta):
	time += delta
	material.set_shader_parameter("wave_time", time)
	
func get_height(world_position: Vector3) -> float:
	# This prevents the crash if the noise hasn't loaded yet
	if noise == null:
		return 0.0
		
	var uv_x = wrapf(world_position.x / noise_scale + time * wave_speed, 0, 1)
	var uv_y = wrapf(world_position.z / noise_scale + time * wave_speed, 0, 1)
	
	var pixel_pos = Vector2(uv_x * noise.get_width(), uv_y * noise.get_height())
	return global_position.y +  noise.get_pixelv(pixel_pos).r * height_scale
