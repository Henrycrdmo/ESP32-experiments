import tkinter as tk
import threading
import asyncio
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque
from bleak import BleakScanner, BleakClient

# --- Variáveis Globais ---
SERVICE_UUID = "00001800-0000-1000-8000-00805f9b34fb"
READ_CHARACTERISTIC_UUID = "00001800-0000-1000-8000-00805f9b34fb"
WRITE_CHARACTERISTIC_UUID = "00001801-0000-1000-8000-00805f9b34fb"
Time = 0
TARGET_ADDRESS = "E8:06:90:66:C0:B2"

# Armazena os valores do sensor (limitado aos últimos 50 pontos)
MAX_POINTS = 50 
data_points = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)
time_stamps = deque(range(MAX_POINTS), maxlen=MAX_POINTS) # Eixo X simples (tempo)

# Variável que armazena a conexão ativa com o ESP32
client = None


# PROBLEMA: A variável client pode causar race conditions
# SOLUÇÃO: Usar uma classe para gerenciar o estado
class BLEManager:
    def __init__(self):
        self.client = None
        self.connected = False
        self.lock = threading.Lock()

ble_manager = BLEManager()
# --- Lógica de Comunicação (Bleak) ---

# Função de callback que será chamada quando o ESP32 enviar um valor
def notification_handler(sender, data):
    """Função chamada quando uma notificação BLE é recebida."""
    try:
        received_string = data.decode('utf-8').strip()
        # Remove caracteres não numéricos (exceto ponto decimal e sinal negativo)
        cleaned_string = ''.join(c for c in received_string if c.isdigit() or c in '.-')
        
        if cleaned_string:
            float_value = float(cleaned_string)
            data_points.append(float_value)
            
            # Atualiza a Label na Thread principal
            root.after(0, lambda: update_plot_ui(float_value))
        else:
            print(f"Dados recebidos vazios ou inválidos: {received_string}")
            
    except ValueError as e:
        print(f"Erro ao converter dados ('{received_string}'): {e}")
    except Exception as e:
        print(f"Erro inesperado no handler: {e}")

async def scan_for_devices(scan_timout):
    """Busca dispositivos BLE e atualiza a lista na UI."""
    # O timeout de 10.0s faz com que o Bleak espere por 10 segundos
    devices = await BleakScanner.discover(timeout=scan_timout) 
    
    # O restante da lógica de preenchimento da lista permanece a mesma...
    for d in devices:
        # Adiciona o dispositivo à lista de forma thread-safe
        if d.name:
            root.after(0, lambda name=d.name, address=d.address: 
                       listbox_devices.insert(tk.END, f"{name} ({address})"))
    
    # Lógica de Finalização (chamada na thread principal após o Bleak terminar) ---
    def finalize_scan():
        btn_scan.config(state=tk.NORMAL)
        global Time
        if Time <= 0:
            status_label.config(text="Busca concluída.")

    root.after(0, finalize_scan)

async def connect_to_ble(address):
    """Conecta ao endereço BLE fornecido."""
    global client
    
    try:
        # Verifica e desconecta client existente de forma mais segura
        if client is not None:
            try:
                await client.disconnect()
            except:
                pass  # Ignora erros se já estiver desconectado
            finally:
                client = None

        client = BleakClient(address)
        await client.connect(timeout=15.0)  # Timeout explícito
        
        if client.is_connected:
            await client.start_notify(READ_CHARACTERISTIC_UUID, notification_handler)
            
            root.after(0, lambda: [
                status_label.config(text=f"CONECTADO a: {address}"),
                btn_connect.config(text="Desconectar", command=desconectar_dispositivo, state=tk.NORMAL),
                btn_send_on.config(state=tk.NORMAL),
                btn_scan.config(state=tk.DISABLED)  # Desativa scan durante conexão
            ])
        else:
            raise Exception("Falha na conexão - não conectado")
            
    except Exception as e:
        root.after(0, lambda: [
            status_label.config(text=f"Falha na conexão: {str(e)[:50]}..."),
            btn_connect.config(state=tk.NORMAL)
        ])
        client = None
    

async def write_to_ble(value_bytes):
    """Escreve um valor na característica BLE."""
    global client
    if client and client.is_connected:
        try:
            # Escreve o comando, como ligar o LED (b'1')
            await client.write_gatt_char(WRITE_CHARACTERISTIC_UUID, value_bytes)
            root.after(0, lambda: status_label.config(text=f"Comando enviado: {value_bytes.decode()}"))
        except Exception as e:
            root.after(0, lambda: status_label.config(text=f"Erro ao enviar comando: {e}"))
    else:
        root.after(0, lambda: status_label.config(text="Erro: Não conectado ao ESP32."))

def update_plot_ui(new_value):
    """Atualiza o gráfico e o label na Thread principal do Tkinter."""
    try:
        # Atualiza os dados
        line.set_ydata(data_points)
        
        # Ajusta dinamicamente os limites do eixo Y se necessário
        if len(data_points) > 0:
            y_min, y_max = min(data_points), max(data_points)
            margin = (y_max - y_min) * 0.1  # 10% de margem
            ax.set_ylim(y_min - margin, y_max + margin)
        
        # Redesenha o gráfico
        canvas.draw_idle()  # Mais eficiente que draw()
        
        # Atualiza o label
        last_value_label.config(text=f"Último Valor: {new_value:.2f}")
        
    except Exception as e:
        print(f"Erro ao atualizar UI: {e}")

# --- Funções de Threading (Lógica de Execução) ---

def run_async_task(coroutine, *args):
    """Executa uma função assíncrona em uma nova thread."""
    def run_in_thread():
        asyncio.run(coroutine(*args))
    
    thread = threading.Thread(target=run_in_thread, daemon=True)
    thread.start()

# --- Funções Ligadas aos Botões (Tkinter) ---

def update_status_countdown():
    """Atualiza o Label de status com a contagem regressiva."""
    global Time
    if Time > 0:
        status_label.config(text=f"Escaneando... {Time}s restantes")
        Time -= 1
        # Agenda a próxima chamada em 1000ms (1 segundo)
        root.after(1000, update_status_countdown)
    else:
        # Quando a contagem chega a zero, o texto de busca concluída será definido pela run_ble_scan
        pass

def iniciar_busca_ble():
    """Chamado pelo botão 'Buscar'. Inicia a busca BLE e o timer na thread."""
    global Time
    
    # 1. Configura a contagem regressiva
    SCAN_DURATION_SECONDS = 10
    Time = SCAN_DURATION_SECONDS
    
    # Limpa a lista antes de escanear novamente
    listbox_devices.delete(0, tk.END)
    
    # 2. Desativa o botão e inicia o timer da UI
    btn_scan.config(state=tk.DISABLED) 
    update_status_countdown() # Inicia o timer da contagem regressiva
    
    # 3. Inicia o BLE (10.0s de timeout)
    run_async_task(scan_for_devices, float(SCAN_DURATION_SECONDS)) 
    
    # A reativação do botão e a mensagem de "Busca concluída"
    # agora serão feitas APÓS o tempo de 10s ter passado no Bleak (dentro de scan_for_devices)

def conectar_dispositivo_direto():
    """Tenta conectar ao endereço MAC definido no TARGET_ADDRESS, ignorando o scan."""
    
    # 1. Verifica se o endereço MAC foi preenchido
    if TARGET_ADDRESS == "00:00:00:00:00:00":
        status_label.config(text="ERRO: Preencha o TARGET_ADDRESS com o MAC real do ESP32!")
        return
        
    address_to_connect = TARGET_ADDRESS

    # 2. Inicia a conexão na thread separada
    run_async_task(connect_to_ble, address_to_connect)
    status_label.config(text=f"Tentando conexão direta com: {address_to_connect}...")

def conectar_dispositivo():
    """Chamado pelo botão 'Conectar'. Inicia a conexão na thread."""
    global client
    
    # Previne múltiplas tentativas de conexão
    if client and client.is_connected:
        status_label.config(text="Já conectado a um dispositivo!")
        return
        
    try:
        selected_item = listbox_devices.get(listbox_devices.curselection())
        address = selected_item.split('(')[-1].strip(')')
        
        btn_connect.config(state=tk.DISABLED)
        status_label.config(text=f"Tentando conectar a: {address}...")
        
        run_async_task(connect_to_ble, address)
        
    except tk.TclError:
        status_label.config(text="Erro: Selecione um dispositivo na lista primeiro!")
    except IndexError:
        status_label.config(text="Erro: Lista vazia ou nenhuma seleção.")

def desconectar_dispositivo():
    """Desconecta do dispositivo BLE."""
    global client
    
    async def disconnect():
        try:
            if client and client.is_connected:
                await client.stop_notify(READ_CHARACTERISTIC_UUID)
                await client.disconnect()
                
        except Exception as e:
            print(f"Erro durante desconexão: {e}")
        finally:
            client = None
            root.after(0, lambda: [
                status_label.config(text="Desconectado."),
                btn_connect.config(text="Conectar", command=conectar_dispositivo, state=tk.NORMAL),
                btn_send_on.config(state=tk.DISABLED),
                btn_scan.config(state=tk.NORMAL),  # Reativa scan
                last_value_label.config(text="Último Valor: N/A")
            ])
    
    run_async_task(disconnect)

def enviar_comando_on():
    """Chamado pelo botão 'LIGAR LED'. Envia o comando '1'."""
    # Envia o byte '1' (b'1') para o ESP32
    run_async_task(write_to_ble, b'1')


# --- Configuração da Janela Principal (UI) ---

root = tk.Tk()
root.title("App Controle BLE (ESP32)")

sensor_label = tk.Label(root, text="Leitura do Sensor: Aguardando...", font=("Arial", 12))
sensor_label.pack(pady=10)

# 1. Rótulo de Status
status_label = tk.Label(root, text="Aperte 'Buscar Dispositivo BLE' para começar.", bd=1, relief=tk.SUNKEN, anchor=tk.W)
status_label.pack(fill=tk.X, pady=(5, 0), padx=5)

# 2. Botões de Ação
frame_buttons = tk.Frame(root)
frame_buttons.pack(pady=10)

btn_scan = tk.Button(frame_buttons, text="1. Buscar Dispositivo BLE", command=iniciar_busca_ble)
btn_scan.pack(side=tk.LEFT, padx=5)

btn_connect = tk.Button(frame_buttons, text="2. Conectar", command=conectar_dispositivo, state=tk.DISABLED)
btn_connect.pack(side=tk.LEFT, padx=5)

btn_connect_direct = tk.Button(root, text="Conexão Direta (MAC)", command=conectar_dispositivo_direto)
btn_connect_direct.pack(side=tk.LEFT, padx=5)

# 3. Lista de Dispositivos
listbox_devices = tk.Listbox(root, width=60, height=10)
listbox_devices.bind('<<ListboxSelect>>', lambda e: btn_connect.config(state=tk.NORMAL)) # Ativa o botão ao selecionar
listbox_devices.pack(padx=10, pady=(0, 10))

# 4. Botão de Comando
btn_send_on = tk.Button(root, text="3. LIGAR LED (Enviar b'1')", state=tk.DISABLED, command=enviar_comando_on)
btn_send_on.pack(pady=5)

# 1. Cria a figura do Matplotlib
fig, ax = plt.subplots(figsize=(6, 3), dpi=100)
# Ajusta o layout para evitar sobreposições
fig.tight_layout(pad=3.0) 

# Configura o eixo Y
ax.set_ylim(0, 100) # Assuma que o sensor vai de 0 a 100
ax.set_title("Monitoramento de Sensor em Tempo Real")
ax.set_ylabel("Valor do Sensor")
ax.set_xlabel("Tempo (últimos 50 pontos)")

# Cria o objeto de linha que será atualizado
line, = ax.plot(time_stamps, data_points)

# 2. Integra a figura ao Tkinter
canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas_widget.pack(padx=10, pady=10)

# 3. Adicione um Label para mostrar o último valor lido (substituindo o antigo sensor_label)
last_value_label = tk.Label(root, text="Último Valor: N/A", font=("Arial", 12, "bold"))
last_value_label.pack(pady=5)

# Inicia o loop da interface gráfica
root.mainloop()

# Opcional: Desconecta automaticamente ao fechar o app
if client and client.is_connected:
    asyncio.run(client.disconnect())