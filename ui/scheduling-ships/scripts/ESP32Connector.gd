extends Node
signal canal_actualizado(datos: Dictionary)

var url := "ws://192.168.4.1/ws"
@export var retry_time := 2.0
var socket: WebSocketPeer = null
var last_state = WebSocketPeer.STATE_CLOSED
var reconnection_timer := 0.0
var _iniciado := false

func _ready() -> void:
	DebugConsole.log_propio("--- Iniciando Conector ESP32 ---")
	DebugConsole.log_propio("Plataforma: " + OS.get_name())
	if OS.get_name() == "Android":
		DebugConsole.log_propio("Solicitando permisos...")
		OS.request_permissions()
		await get_tree().create_timer(4.0).timeout
		DebugConsole.log_propio("Listo, conectando...")
	_iniciado = true
	_connect_socket()

func _connect_socket():
	if socket != null:
		socket.close()
		socket = null

	socket = WebSocketPeer.new()
	DebugConsole.log_propio("Conectando a: " + url)
	var err = socket.connect_to_url(url)
	DebugConsole.log_propio("Resultado: %d" % err)

	if err != OK:
		DebugConsole.log_error("Error al conectar: %d — reintentando en %s s" % [err, str(retry_time)])
		socket = null
		return
	last_state = WebSocketPeer.STATE_CONNECTING

func _process(delta: float) -> void:
	if not _iniciado:
		return

	# Sin socket → esperar y reintentar
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
			DebugConsole.log_error("Socket cerrado. Reconectando en %.1f s..." % retry_time)
			socket = null
			last_state = WebSocketPeer.STATE_CLOSED
			reconnection_timer = 0.0

func _on_state_changed(new_state):
	match new_state:
		WebSocketPeer.STATE_CONNECTING: DebugConsole.log_propio("WebSocket: Conectando...")
		WebSocketPeer.STATE_OPEN:       DebugConsole.log_ok("WebSocket: ¡Conectado!")
		WebSocketPeer.STATE_CLOSING:    DebugConsole.log_error("WebSocket: Cerrando (ESP32 reiniciado?)...")
		WebSocketPeer.STATE_CLOSED:     pass  # ya lo maneja el match de arriba

func _procesar_paquete(json_string: String):
	var json = JSON.new()
	var error = json.parse(json_string)
	if error == OK:
		print(json.data)
		canal_actualizado.emit(json.data)
	else:
		DebugConsole.log_error("PARSEO: " + json.get_error_message())
