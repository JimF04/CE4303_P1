extends Node

@export var url := "ws://192.168.5.136/ws"
var socket := WebSocketPeer.new()

func _ready() -> void:
	print("--- Iniciando Monitor de Datos ESP32 ---")
	var err = socket.connect_to_url(url)
	if err != OK:
		print("Error al intentar conectar")
	else:
		print("Intentando conectar a: ", url)

func _process(_delta: float) -> void:
	socket.poll()
	var state = socket.get_ready_state()
	
	if state == WebSocketPeer.STATE_OPEN:
		while socket.get_available_packet_count() > 0:
			var packet = socket.get_packet()
			var json_string = packet.get_string_from_utf8()
			_imprimir_todo(json_string)
			
	elif state == WebSocketPeer.STATE_CLOSED:
		# Reconexión automática
		socket.connect_to_url(url)

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
		
		# 4. Lista del Scheduler (Ordenado)
		print("\nCOLA DE ESPERA (IDs):")
		if data["ordenado"].size() > 0:
			print("  " + str(data["ordenado"]))
		else:
			print("  (Vasta o vacía)")
			
		print("=".repeat(40) + "\n")
	else:
		print("ERROR DE PARSEO: ", json.get_error_message())
		print("Contenido conflictivo: ", json_string)
