extends Node3D

# ==============================================================================
# CONFIGURACIÓN DEL CANAL
# ==============================================================================
@export var canal_largo_visual: float = 56.0        # más espacio entre slots
@export var canal_origen: Vector3 = Vector3(0.0, 0.0, -28.0)

@export var rotacion_derecha_deg: float   = 180.0
@export var rotacion_izquierda_deg: float =   0.0

@export var duracion_movimiento: float = 0.48

# ==============================================================================
# ESCENAS DE BARCOS
# ==============================================================================
var escenas_barco = {
	"NOR": preload("res://scenes/barco_normal.tscn"),
	"PES": preload("res://scenes/barco_pesquera.tscn"),
	"PAT": preload("res://scenes/barco_patrulla.tscn"),
}

# ==============================================================================
# ESTADO INTERNO
# ==============================================================================
var barcos_activos: Dictionary = {}   # id -> Node3D
var tweens_activos: Dictionary = {}   # id -> Tween
var rotacion_actual: float = deg_to_rad(0.0)  # última rotación aplicada
var ultimo_estado: Dictionary = {}

@onready var contenedor_barcos: Node3D = $Barcos
@onready var conector: Node = $ESP32Connector

# ==============================================================================
# READY
# ==============================================================================
func _ready() -> void:
	call_deferred("_conectar_señales")

func _conectar_señales() -> void:
	if conector == null:
		push_error("No se encontró ESP32Connector.")
		return
	if not conector.has_signal("canal_actualizado"):
		push_error("Señal 'canal_actualizado' no encontrada.")
		return
	conector.canal_actualizado.connect(_on_canal_actualizado)
	print("Señal conectada.")

# ==============================================================================
# CALLBACK PRINCIPAL
# ==============================================================================
func _on_canal_actualizado(datos: Dictionary) -> void:
	ultimo_estado = datos
	_sincronizar_barcos(datos)

# ==============================================================================
# SINCRONIZACIÓN
# ==============================================================================
func _sincronizar_barcos(datos: Dictionary) -> void:
	var slots: Array = datos.get("slots", [])
	var num_slots: int = slots.size()
	if num_slots == 0:
		return

	var dir: int = int(datos.get("dir", 0))
	var nueva_rot_rad: float = deg_to_rad(
		rotacion_derecha_deg if dir == 1 else rotacion_izquierda_deg
	)
	var dir_cambio: bool = not is_equal_approx(nueva_rot_rad, rotacion_actual)
	if dir_cambio:
		rotacion_actual = nueva_rot_rad

	var espaciado: float = canal_largo_visual / max(num_slots - 1, 1)

	# -- 1. Qué IDs deben existir --
	var ids_esperados: Dictionary = {}
	for i in range(num_slots):
		var slot = slots[i]
		if slot == null:
			continue
		var barco_id: int = int(slot["id"])
		ids_esperados[barco_id] = {"tipo": slot["tipo"], "slot": i}

	# -- 2. Eliminar los que salieron --
	var ids_a_eliminar: Array = []
	for id in barcos_activos:
		if not ids_esperados.has(id):
			ids_a_eliminar.append(id)
	for id in ids_a_eliminar:
		_cancelar_tween(id)
		barcos_activos[id].queue_free()
		barcos_activos.erase(id)

	# -- 3. Crear o animar --
	for barco_id in ids_esperados:
		var info = ids_esperados[barco_id]
		var pos_destino: Vector3 = _posicion_de_slot(info["slot"], espaciado)

		if barcos_activos.has(barco_id):
			_animar_movimiento(barco_id, pos_destino, nueva_rot_rad, dir_cambio)
		else:
			var nodo = _crear_barco(barco_id, info["tipo"], pos_destino, nueva_rot_rad)
			if nodo:
				barcos_activos[barco_id] = nodo

# ==============================================================================
# TWEEN
# ==============================================================================
func _animar_movimiento(barco_id: int, pos_destino: Vector3, rot_y_rad: float, rotar: bool) -> void:
	var nodo: Node3D = barcos_activos[barco_id]
	_cancelar_tween(barco_id)

	var t = create_tween()

	if rotar:
		# Solo rotamos si la dirección del canal cambió
		t.set_parallel(true)
		t.tween_property(nodo, "position", pos_destino, duracion_movimiento)\
			.set_trans(Tween.TRANS_LINEAR)
		t.tween_property(nodo, "rotation:y", rot_y_rad, duracion_movimiento * 0.4)\
			.set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	else:
		# Solo movimiento, sin tocar la rotación
		t.tween_property(nodo, "position", pos_destino, duracion_movimiento)\
			.set_trans(Tween.TRANS_LINEAR)

	tweens_activos[barco_id] = t

func _cancelar_tween(barco_id: int) -> void:
	if tweens_activos.has(barco_id):
		var t = tweens_activos[barco_id]
		if t and t.is_valid():
			t.kill()
		tweens_activos.erase(barco_id)

# ==============================================================================
# HELPERS
# ==============================================================================
func _posicion_de_slot(slot_idx: int, espaciado: float) -> Vector3:
	return canal_origen + Vector3(0.0, 0.0, slot_idx * espaciado)

func _crear_barco(barco_id: int, tipo: String, pos: Vector3, rot_y_rad: float) -> Node3D:
	if not escenas_barco.has(tipo):
		push_warning("Tipo desconocido: " + tipo)
		return null
	var nodo: Node3D = escenas_barco[tipo].instantiate()
	nodo.name = "Barco_%d" % barco_id
	nodo.position = pos
	nodo.rotation.y = rot_y_rad
	contenedor_barcos.add_child(nodo)
	return nodo

# ==============================================================================
# TEST
# ==============================================================================
func _input(event):
	if not event is InputEventKey or not event.pressed:
		return
	match event.keycode:
		KEY_T: _test_paquete_fake(0)
		KEY_D: _test_paquete_fake(1)
		KEY_M: _test_mover_barcos()

func _test_paquete_fake(dir_test: int):
	_on_canal_actualizado({
		"buque_act": -1.0, "dir": float(dir_test),
		"ordenado_der": [3.0, 4.0, 5.0], "ordenado_izq": [],
		"slots": [
			null, null, null, null, null, null, null, null,
			{"id": 2.0, "tipo": "PES"},
			{"id": 1.0, "tipo": "PAT"},
			null,
			{"id": 0.0, "tipo": "NOR"},
			null, null, null
		]
	})

func _test_mover_barcos():
	_on_canal_actualizado({
		"buque_act": -1.0, "dir": 0.0,
		"ordenado_der": [3.0, 4.0, 5.0], "ordenado_izq": [],
		"slots": [
			null, null, null, null, null, null,
			{"id": 2.0, "tipo": "PES"},
			null, null, null,
			{"id": 0.0, "tipo": "NOR"},
			null, null, null, null
		]
	})
