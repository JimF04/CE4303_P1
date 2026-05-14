extends Node3D

# ==============================================================================
# CONFIGURACIÓN DEL CANAL
# ==============================================================================
@export var canal_largo_visual: float = 35.0
@export var suelo_y: float = 0.0
@export var canal_origen: Vector3 = Vector3(0.0, 0.0, -17.5)

@export var rotacion_derecha_deg: float   = 180.0
@export var rotacion_izquierda_deg: float =   0.0

# Debe coincidir con el intervalo del ESP32 / fake connector
@export var intervalo_seg: float = 0.5

# ==============================================================================
# LISTAS DE ESPERA  (fila en X, perpendicular al canal)
# ==============================================================================
@export var lista_offset_x: float     = 5.0
@export var lista_separacion_x: float = 3.0

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
var barcos_activos: Dictionary = {}   # bid -> Node3D
var tweens_activos: Dictionary = {}   # bid -> Tween
var tipos_conocidos: Dictionary = {}  # bid -> String

var rotacion_actual: float = deg_to_rad(0.0)

# Buffer de un tick
var frame_pendiente: Dictionary = {}
var hay_frame_pendiente: bool = false

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
# CALLBACK — lag de un tick
# Al llegar un frame nuevo, animamos el frame anterior (ya conocemos el destino
# exacto) con exactamente intervalo_seg de duración. Así el tween siempre
# termina justo cuando llega el siguiente frame.
# ==============================================================================
func _on_canal_actualizado(datos: Dictionary) -> void:
	if not hay_frame_pendiente:
		# Primer frame: solo registramos tipos y lo guardamos, nada más
		_registrar_tipos(datos)
		frame_pendiente = datos
		hay_frame_pendiente = true
		return

	# Animamos hacia el frame pendiente con duración = intervalo exacto
	_sincronizar_barcos(frame_pendiente, intervalo_seg)

	# El frame recién llegado pasa a ser el pendiente
	_registrar_tipos(datos)
	frame_pendiente = datos

# ==============================================================================
# REGISTRAR TIPOS — separado para poder hacerlo también en el primer frame
# ==============================================================================
func _registrar_tipos(datos: Dictionary) -> void:
	for s in datos.get("slots", []):
		if s != null:
			tipos_conocidos[int(s["id"])] = s["tipo"]
	for entry in datos.get("ordenado_izq", []):
		if entry is Dictionary:
			tipos_conocidos[int(entry["id"])] = entry["tipo"]
	for entry in datos.get("ordenado_der", []):
		if entry is Dictionary:
			tipos_conocidos[int(entry["id"])] = entry["tipo"]

# ==============================================================================
# SINCRONIZACIÓN
# ==============================================================================
func _sincronizar_barcos(datos: Dictionary, dur: float) -> void:
	var slots: Array     = datos.get("slots", [])
	var lista_izq: Array = datos.get("ordenado_izq", [])
	var lista_der: Array = datos.get("ordenado_der", [])
	var num_slots: int   = slots.size()
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

	# ── 1. Sets de IDs esperados ─────────────────────────────────────────────
	var ids_en_canal: Dictionary = {}
	for i in range(num_slots):
		if slots[i] != null:
			ids_en_canal[int(slots[i]["id"])] = i

	var ids_en_lista_izq: Dictionary = {}
	for i in range(lista_izq.size()):
		var entry = lista_izq[i]
		var bid: int = int(entry["id"]) if entry is Dictionary else int(entry)
		ids_en_lista_izq[bid] = i

	var ids_en_lista_der: Dictionary = {}
	for i in range(lista_der.size()):
		var entry = lista_der[i]
		var bid: int = int(entry["id"]) if entry is Dictionary else int(entry)
		ids_en_lista_der[bid] = i

	# ── 2. Eliminar desaparecidos ────────────────────────────────────────────
	var ids_a_eliminar: Array = []
	for bid in barcos_activos:
		if not ids_en_canal.has(bid) \
		and not ids_en_lista_izq.has(bid) \
		and not ids_en_lista_der.has(bid):
			ids_a_eliminar.append(bid)
	for bid in ids_a_eliminar:
		_cancelar_tween(bid)
		barcos_activos[bid].queue_free()
		barcos_activos.erase(bid)
		tipos_conocidos.erase(bid)


# ── 3. Lista izquierda ───────────────────────────────────────────────────
	var rot_izq := deg_to_rad(rotacion_izquierda_deg)
	for bid in ids_en_lista_izq:
		var orden_original: int = ids_en_lista_izq[bid]
		var pos := _posicion_lista_izq(orden_original)
		if not barcos_activos.has(bid):
			var nodo := _crear_barco(bid, tipos_conocidos.get(bid, "NOR"), pos, rot_izq)
			if nodo:
				barcos_activos[bid] = nodo
		else:
			# Si venía del canal -> teletransporte, si estaba en lista -> tween
			if ids_en_canal.has(bid):
				_cancelar_tween(bid)
				barcos_activos[bid].position = pos
				barcos_activos[bid].rotation.y = rot_izq
			else:
				_animar_movimiento_dur(bid, pos, rot_izq, false, dur)

	# ── 4. Lista derecha ─────────────────────────────────────────────────────
	var rot_der := deg_to_rad(rotacion_derecha_deg)
	for bid in ids_en_lista_der:
		var pos := _posicion_lista_der(ids_en_lista_der[bid])
		if not barcos_activos.has(bid):
			var nodo := _crear_barco(bid, tipos_conocidos.get(bid, "NOR"), pos, rot_der)
			if nodo:
				barcos_activos[bid] = nodo
		else:
			# Si venía del canal -> teletransporte, si estaba en lista -> tween
			if ids_en_canal.has(bid):
				_cancelar_tween(bid)
				barcos_activos[bid].position = pos
				barcos_activos[bid].rotation.y = rot_der
			else:
				_animar_movimiento_dur(bid, pos, rot_der, false, dur)

	# ── 5. Canal ─────────────────────────────────────────────────────────────
	for bid in ids_en_canal:
		var pos_destino := _posicion_de_slot(ids_en_canal[bid], espaciado)
		if barcos_activos.has(bid):
			# Anima desde donde está (lista o slot anterior) al slot destino.
			# Duración = intervalo_seg → llega exacto cuando llega el próximo frame.
			_animar_movimiento_dur(bid, pos_destino, nueva_rot_rad, dir_cambio, dur)
		else:
			var tipo: String = tipos_conocidos.get(bid, "NOR")
			var nodo := _crear_barco(bid, tipo, pos_destino, nueva_rot_rad)
			if nodo:
				barcos_activos[bid] = nodo

# ==============================================================================
# POSICIONES DE LISTA
# ==============================================================================
func _posicion_lista_izq(orden: int) -> Vector3:
	return Vector3(
		canal_origen.x + lista_offset_x + orden * lista_separacion_x,
		suelo_y,
		canal_origen.z
	)

func _posicion_lista_der(orden: int) -> Vector3:
	return Vector3(
		canal_origen.x + lista_offset_x + orden * lista_separacion_x,
		suelo_y,
		canal_origen.z + canal_largo_visual
	)

# ==============================================================================
# TWEEN
# ==============================================================================
func _animar_movimiento_dur(barco_id: int, pos_destino: Vector3, rot_y_rad: float, rotar: bool, dur: float) -> void:
	var nodo: Node3D = barcos_activos[barco_id]
	_cancelar_tween(barco_id)
	var t := create_tween()
	if rotar:
		t.set_parallel(true)
		t.tween_property(nodo, "position", pos_destino, dur) \
			.set_trans(Tween.TRANS_LINEAR)
		t.tween_property(nodo, "rotation:y", rot_y_rad, dur * 0.4) \
			.set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	else:
		t.tween_property(nodo, "position", pos_destino, dur) \
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
	return Vector3(canal_origen.x, suelo_y, canal_origen.z + slot_idx * espaciado)

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
		KEY_F: _test_canal()

func _test_canal():
	_on_canal_actualizado({
		"buque_act": -1.0, "dir": 0.0,
		"ordenado_izq": [],
		"ordenado_der": [
			{"id": 3.0, "tipo": "PAT"},
			{"id": 4.0, "tipo": "PES"},
			{"id": 5.0, "tipo": "NOR"},
		],
		"slots": [
			{"id": 1.0, "tipo": "PAT"},
			null, null, null, null, null,
			{"id": 2.0, "tipo": "NOR"},
			null, null, null, null,
			null, null, null,
			{"id": 3.0, "tipo": "PES"},
		]
	})

func _test_paquete_fake(dir_test: int):
	_on_canal_actualizado({
		"buque_act": -1.0, "dir": float(dir_test),
		"ordenado_izq": [],
		"ordenado_der": [
			{"id": 3.0, "tipo": "PAT"},
			{"id": 4.0, "tipo": "PES"},
			{"id": 5.0, "tipo": "NOR"},
		],
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
		"ordenado_izq": [],
		"ordenado_der": [
			{"id": 3.0, "tipo": "PAT"},
			{"id": 4.0, "tipo": "PES"},
			{"id": 5.0, "tipo": "NOR"},
		],
		"slots": [
			null, null, null, null, null, null,
			{"id": 2.0, "tipo": "PES"},
			null, null, null,
			{"id": 0.0, "tipo": "NOR"},
			null, null, null, null
		]
	})
