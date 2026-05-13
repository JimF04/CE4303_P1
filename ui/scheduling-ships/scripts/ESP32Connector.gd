extends Node

signal canal_actualizado(datos: Dictionary)

@export var url = "ws://canal.local/ws"
@export var retry_time = 3.0

var socket: WebSocketPeer = null
var last_state = WebSocketPeer.STATE_CLOSED
var reconnection_timer = 0.0

func _ready() -> void:
	print("--- Iniciando Conector ESP32 ---")
	_connect_socket()

func _connect_socket():
	# crear instancia nueva cada vez — WebSocketPeer no es reutilizable
	socket = WebSocketPeer.new()
	socket.inbound_buffer_size = 1024 * 128
	socket.max_queued_packets = 2048

	print("Intentando conectar a: ", url)
	var err = socket.connect_to_url(url)
	if err != OK:
		print("Error al intentar conectar: ", err)
		socket = null
		return

	last_state = WebSocketPeer.STATE_CONNECTING

func _process(delta: float) -> void:
	if socket == null:
		reconnection_timer += delta
		if reconnection_timer >= retry_time:
			reconnection_timer = 0.0
			_connect_socket()
		return

	socket.poll()
	var state = socket.get_ready_state()

	if state != last_state:
		_on_state_changed(state)
		last_state = state

	match state:
		WebSocketPeer.STATE_OPEN:
			reconnection_timer = 0.0
			while socket.get_available_packet_count() > 0:
				var packet = socket.get_packet()
				if socket.was_string_packet():
					var json_string = packet.get_string_from_utf8()
					if json_string.length() > 0:
						_procesar_paquete(json_string)

		WebSocketPeer.STATE_CLOSED:
			# Socket muerto → descartarlo y esperar para reconectar
			socket = null
			reconnection_timer = 0.0
			print("Socket descartado. Reconectando en %.1fs..." % retry_time)

func _on_state_changed(new_state):
	match new_state:
		WebSocketPeer.STATE_CONNECTING: print("WebSocket: Conectando...")
		WebSocketPeer.STATE_OPEN:       print("WebSocket: ¡Conectado!")
		WebSocketPeer.STATE_CLOSING:    print("WebSocket: Cerrando (ESP32 reiniciado?)...")
		WebSocketPeer.STATE_CLOSED:     print("WebSocket: Cerrado.")

func _procesar_paquete(json_string: String):
	var json = JSON.new()
	var error = json.parse(json_string)
	if error == OK:
		canal_actualizado.emit(json.data)
	else:
		print("ERROR DE PARSEO: ", json.get_error_message())
