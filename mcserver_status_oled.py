import network
import urequests
import json
from machine import Pin, I2C
import time

#OLED
i2c = I2C(scl=Pin(4), sda=Pin(5))
from ssd1306 import SSD1306_I2C 
OLED= SSD1306_I2C(128, 64, i2c)

# 设置WiFi网络参数
SSID = 'your_ssid' 
PASSWORD = 'your_password' 

# API Key 和 URL
api_key = "your_mcsmanager_api_key"
server_ip = "your_server_ip"
REMOTE_SERVICES_API_URL = f"http://your.mcsmanager.ip/api/service/remote_services_system?apikey={api_key}"
MC_SERVER_API_URL = f"http://mcstatus.goldenapplepie.xyz/api/?ip={server_ip}"

# 连接到WiFi
def connect_to_wifi(ssid, password, oled, max_retries=5, retry_delay=5):
    wlan = network.WLAN(network.STA_IF) 
    wlan.active(True) 
    
    retries = 0
    while not wlan.isconnected() and retries < max_retries:
        OLED.fill(0)  
        OLED.text('Connecting...', 20, 30) 
        OLED.show()
        print('Connecting to network...')
        try:
            wlan.connect(ssid, password)  
            for _ in range(10):  
                if wlan.isconnected():
                    break
                time.sleep(1)
            else: 
                raise OSError("Unable to connect to WiFi")
        except OSError as e:
            print('WiFi connection error:', e)
            OLED.fill(0)  
            OLED.text('[ERROR]',32,18)
            OLED.text('Connection', 20, 30) 
            OLED.text('Failed', 36,40)
            OLED.show()
            time.sleep(retry_delay)  
            retries += 1
    
    if wlan.isconnected():
        OLED.fill(0)  
        print('Network config:', wlan.ifconfig())  
    else:
        OLED.fill(0)  
        OLED.text('Unable to connect', 0, 30)  
        OLED.show()
        print('Unable to connect to WiFi after', max_retries, 'retries.')

# 获取远程服务器数据
def fetch_remote_services_data(api_url, oled):
    try:
        response = urequests.get(api_url)
        data = response.json()
        if data['status'] == 200:
            node_data = data['data'][0]
            cpu_usage = node_data['system']['cpuUsage'] * 100  
            mem_usage = node_data['system']['memUsage'] * 100  
            running_instances = node_data['instance']['running']
            total_instances = node_data['instance']['total']
            response.close()
            return cpu_usage, mem_usage, running_instances, total_instances
        else:
            print(f"Error: API returned status {data['status']}")
            OLED.fill(0)  
            OLED.text('MCSManager Data', 0, 5)
            OLED.text('------------------', 0, 15)
            OLED.text(f'API Error: {data["status"]}',10, 40)
            OLED.show()
            time.sleep(2)
            return None, None, None, None
    except Exception as e:
        print("Error fetching data:", e)
        OLED.fill(0)  
        OLED.text('Connection Error', 0, 30)
        OLED.show()
        time.sleep(2)
        return None, None, None, None

# 获取MC服务器状态
def check_mc_server_status(url,oled):
    try:
        response = urequests.get(url)
        data = response.json()
        code = data.get("code")
        if code == 200:
            players = data.get("data", {}).get("players", {})
            online = players.get("online", 0)
            max_players = players.get("max", 0)
            server_status = "在线"
        elif code == 204:
            server_status = "离线"
            online = 0
            max_players = 0
        else:
            server_status = "未知状态"
            online = 0
            max_players = 0
        response.close()
        return server_status, online, max_players
    except Exception as e:
        print("查询过程中出现错误:", e)
        OLED.fill(0) 
        OLED.text('Connection Error', 0, 30) 
        OLED.show()
        time.sleep(2)  
        return "Error", 0, 0  
# 主程序
def main():
    connect_to_wifi(SSID, PASSWORD, OLED, max_retries=10, retry_delay=5)  
    
    task_counter = 0  
    while True:  
        if task_counter % 2 == 0:
            server_status, online, max_players = check_mc_server_status(MC_SERVER_API_URL, OLED)
            display_mc_server_status(server_status, online, max_players)
        else:
            cpu_usage, mem_usage, running, total = fetch_remote_services_data(REMOTE_SERVICES_API_URL, OLED)
            display_remote_services_data(cpu_usage, mem_usage, running, total)
        
        task_counter += 1
        time.sleep(5)  # 等待5秒
# 显示MC服务器状态
def display_mc_server_status(server_status, online, max_players):
    OLED.fill(0)  
    OLED.text('MC Server Status', 0, 5)
    OLED.text('------------------', 0, 15)
    OLED.text('Server:', 0, 30)
    if server_status == "在线":
        server_status = "Online"
    elif server_status == "离线":
        server_status = "Offline"
    elif server_status == "未知状态":
        server_status = "Unknown"
    OLED.text(server_status, 60, 30)
    OLED.text('Players:', 0, 45)
    OLED.text(f'{online}/{max_players}', 65, 45)
    OLED.show()
# 显示远程服务器数据
def display_remote_services_data(cpu_usage, mem_usage, running, total):
    if cpu_usage is not None and mem_usage is not None and running is not None and total is not None:
        OLED.fill(0)
        OLED.text('MCSManager Data', 0, 5)
        OLED.text('------------------', 0, 15)
        OLED.text(f'CPU: {cpu_usage:.2f}%', 0, 25)
        OLED.text(f'Mem: {mem_usage:.2f}%', 0, 40)
        OLED.text(f'Instance: {running}/{total}', 0, 55)
        OLED.show()
main()



