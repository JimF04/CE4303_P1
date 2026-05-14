extends Node

# ==============================================================================
# ESP32 FAKE — emite canal_actualizado con frames hardcodeados
# Reemplazá el nodo ESP32Connector con este script para pruebas sin hardware.
# ==============================================================================

signal canal_actualizado(datos: Dictionary)

@export var intervalo: float = 0.5   # segundos entre frames
@export var loop: bool = true        # volver al inicio al terminar

var _frames: Array[Dictionary] = []
var _idx: int = 0
var _timer: float = 0.0
var _activo: bool = true

# ==============================================================================
# FRAMES — pegá acá tus líneas JSON (una por elemento)
# ==============================================================================
const FRAMES_RAW: Array = [
	{"dir":0.0,"buque_act":1.0,"slots":[null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[{"id":1,"tipo":"PAT"}, {"id":2,"tipo":"PES"}],"ordenado_der": [{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":1.0,"slots":[null,null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[{"id":1,"tipo":"PAT"}, {"id":2,"tipo":"PES"}],"ordenado_der": [{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":1.0,"slots":[null,null,null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[{"id":1,"tipo":"PAT"}, {"id":2,"tipo":"PES"}],"ordenado_der": [{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":2.0,"slots":[null,null,null,{"id":1.0,"tipo":"PAT"},{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[{"id":2,"tipo":"PES"}],"ordenado_der": [{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,{"id":2.0,"tipo":"PES"},{"id":1.0,"tipo":"PAT"},null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,{"id":2.0,"tipo":"PES"},{"id":1.0,"tipo":"PAT"},null,null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,{"id":2.0,"tipo":"PES"},null,{"id":1.0,"tipo":"PAT"},{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,{"id":2.0,"tipo":"PES"},null,{"id":1.0,"tipo":"PAT"},null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,{"id":2.0,"tipo":"PES"},null,{"id":1.0,"tipo":"PAT"},null,null,{"id":0.0,"tipo":"NOR"},null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,{"id":2.0,"tipo":"PES"},null,null,{"id":1.0,"tipo":"PAT"},{"id":0.0,"tipo":"NOR"},null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,{"id":2.0,"tipo":"PES"},{"id":1.0,"tipo":"PAT"},null,{"id":0.0,"tipo":"NOR"},null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,{"id":2.0,"tipo":"PES"},{"id":1.0,"tipo":"PAT"},null,null,{"id":0.0,"tipo":"NOR"},null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,{"id":2.0,"tipo":"PES"},null,{"id":1.0,"tipo":"PAT"},{"id":0.0,"tipo":"NOR"},null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":0.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,{"id":2.0,"tipo":"PES"},null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":3,"tipo":"PAT"}, {"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":1.0,"buque_act":4.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,{"id":3.0,"tipo":"PAT"},null,null,null],"ordenado_izq":[],"ordenado_der":[{"id":4,"tipo":"PES"}, {"id":5,"tipo":"NOR"}]},
	{"dir":1.0,"buque_act":5.0,"slots":[null,null,null,null,null,null,null,null,{"id":3.0,"tipo":"PAT"},null,null,null,{"id":4.0,"tipo":"PES"},null,null],"ordenado_izq":[],"ordenado_der":[{"id":5,"tipo":"NOR"}]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,{"id":3.0,"tipo":"PAT"},null,null,null,null,{"id":4.0,"tipo":"PES"},null,null,{"id":5.0,"tipo":"NOR"},null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,{"id":3.0,"tipo":"PAT"},null,null,null,null,null,{"id":4.0,"tipo":"PES"},null,null,null,{"id":5.0,"tipo":"NOR"},null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,{"id":4.0,"tipo":"PES"},null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,{"id":4.0,"tipo":"PES"},null,null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,{"id":4.0,"tipo":"PES"},null,null,null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":1.0,"buque_act":-1.0,"slots":[null,{"id":5.0,"tipo":"NOR"},null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
	{"dir":-1.0,"buque_act":-1.0,"slots":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null],"ordenado_izq":[],"ordenado_der":[]},
]

# ==============================================================================
# READY / PROCESS
# ==============================================================================
func _ready() -> void:
	_frames.assign(FRAMES_RAW)
	print("[FakeESP32] %d frames cargados. Intervalo: %.2fs" % [_frames.size(), intervalo])

func _process(delta: float) -> void:
	if not _activo or _frames.is_empty():
		return
	_timer += delta
	if _timer >= intervalo:
		_timer = 0.0
		_emitir_frame()

func _emitir_frame() -> void:
	canal_actualizado.emit(_frames[_idx])
	print("[FakeESP32] frame %d / %d" % [_idx, _frames.size() - 1])
	_idx += 1
	if _idx >= _frames.size():
		if loop:
			_idx = 0
		else:
			_activo = false
			print("[FakeESP32] secuencia terminada.")
