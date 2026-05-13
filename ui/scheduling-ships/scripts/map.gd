extends Node3D
# 1. Cargamos la escena del barco en una variable
# Arrastra el archivo .tscn del barco desde el sistema de archivos al código para obtener la ruta
var barco_escena = preload("res://scenes/barco_normal.tscn")

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _input(event):
	# 2. Verificamos si se presionó una tecla (ejemplo: la tecla "E")
	# Nota: Es mejor usar Input Map en Project Settings, pero esto sirve para pruebas rápidas
	if event is InputEventKey and event.pressed and event.keycode == KEY_E:
		generar_barco()
		
func generar_barco():
	# 3. Creamos una instancia (copia) del barco
	var nuevo_barco = barco_escena.instantiate()
	
	# 4. Definimos dónde aparecerá
	# Puedes usar una posición fija o un nodo "Marker3D" que ya tengas en el mapa
	var posicion_spawn = Vector3(0, 5, 0) # X, Y (altura), Z
	nuevo_barco.position = posicion_spawn
	
	# 5. Lo añadimos a la escena
	add_child(nuevo_barco)
	
	print("¡Barco generado!")
