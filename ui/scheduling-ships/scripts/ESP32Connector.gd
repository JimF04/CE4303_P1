extends Node

@export var url = "ws://canal.local/ws"
@export var retry_time = 1.0  # Segundos entre reintentos

var socket := WebSocketPeer.new()
var last_state = WebSocketPeer.STATE_CLOSED
var reconnection_timer = 0.0

func _ready() -> void:
	print("--- Iniciando Monitor de Datos ESP32 ---")
	_connect()

func _connect():
	print("Intentando conectar a: ", url)
	# IMPORTANTE: Aumentar buffers antes de conectar para evitar que el JSON se corte
	socket.inbound_buffer_size = 1024 * 128 # 128KB
	socket.max_queued_packets = 2048
	
	var err = socket.connect_to_url(url)
	if err != OK:
		print("Error inmediato al intentar conectar: ", err)

func _process(delta: float) -> void:
	socket.poll()
	var state = socket.get_ready_state()
	
	if state != last_state:
		_on_state_changed(state)
		last_state = state

	if state == WebSocketPeer.STATE_OPEN:
		reconnection_timer = 0.0
		
		# Mientras haya paquetes, los procesamos
		while socket.get_available_packet_count() > 0:
			var packet = socket.get_packet()
			if socket.was_string_packet(): # Asegurarnos de que es texto
				var json_string = packet.get_string_from_utf8()
				
				# Validar que el string no sea nulo o vacío por error de transmisión
				if json_string.length() > 0:
					_imprimir_todo(json_string)
			else:
				print("Recibido paquete binario inesperado")
			
func _on_state_changed(new_state):
	match new_state:
		WebSocketPeer.STATE_CONNECTING:
			print("Estado: Conectando...")
		WebSocketPeer.STATE_OPEN:
			print("Estado: ¡Conectado con éxito!")
		WebSocketPeer.STATE_CLOSING:
			print("Estado: Cerrando conexión...")
		WebSocketPeer.STATE_CLOSED:
			print("Estado: Desconectado.")

func _imprimir_todo(json_string: String):
	var json = JSON.new()
	var error = json.parse(json_string)
	
	if error == OK:
		var data = json.data
		
		print("\n" + "=".repeat(40))
		print("NUEVO PAQUETE RECIBIDO")
		print("=".repeat(40))
		
		# 1. JSON Crudo formateado (Pretty Print)
		print("JSON COMPLETO:")
		print(JSON.stringify(data, "\t"))
		
		print("-".repeat(20))
		
		# 2. Desglose manual de datos
		print("DIRECCIÓN ACTIVA: ", "Derecha (1)" if data["dir"] == 1 else "Izquierda (0)")
		print("BUQUE PRÓXIMO (ID): ", data["buque_act"])
		
		# 3. Listado de barcos en el canal (Slots)
		print("\nESTADO DE LOS SLOTS:")
		var slots = data["slots"]
		for i in range(slots.size()):
			if slots[i] == null:
				print("  [%d]: -- Vacío --" % i)
			else:
				var b = slots[i]
				print("  [%d]: Barco ID %d (Tipo: %s)" % [i, b["id"], b["tipo"]])
		

		# 4. Lista del Scheduler (Ordenado por lados)
		print("\nCOLAS DE ESPERA (IDs):")
		
		# Lado Izquierdo
		if data["ordenado_izq"].size() > 0:
			print("  IZQUIERDA: " + str(data["ordenado_izq"]))
		else:
			print("  IZQUIERDA: (Vacía)")
			
		# Lado Derecho
		if data["ordenado_der"].size() > 0:
			print("  DERECHA:   " + str(data["ordenado_der"]))
		else:
			print("  DERECHA:   (Vacía)")
			
		print("=".repeat(40) + "\n")
	else:
		print("ERROR DE PARSEO: ", json.get_error_message())
		print("Contenido conflictivo: ", json_string)
