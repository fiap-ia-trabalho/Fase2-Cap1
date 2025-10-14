import requests
import json
import time
from datetime import datetime

# --- CONFIGURAÇÃO DA API ---
# Use a sua chave da OpenWeatherMap
API_KEY = "77b63a671563949ddd2ab6f5357fbb99" 
URL_BASE = "http://api.openweathermap.org/data/2.5/weather"

# Vetor com as Capitais e suas respectivas latitudes e longitudes.

def buscar_clima_capital():
    MAPA_COORDENADAS = {     "AC": {"cidade": "Rio Branco", "lat": -9.9724, "lon": -67.8101},     "AL": {"cidade": "Maceió", "lat": -9.6432, "lon": -35.7188},     "AP": {"cidade": "Macapá", "lat": 0.0333, "lon": -51.0583},     "AM": {"cidade": "Manaus", "lat": -3.0915, "lon": -60.0214},     "BA": {"cidade": "Salvador", "lat": -12.9711, "lon": -38.5108},     "CE": {"cidade": "Fortaleza", "lat": -3.7319, "lon": -38.5267},     "DF": {"cidade": "Brasília", "lat": -15.7797, "lon": -47.9297},     "ES": {"cidade": "Vitória", "lat": -20.3177, "lon": -40.3367},     "GO": {"cidade": "Goiânia", "lat": -16.6869, "lon": -49.2656},     "MA": {"cidade": "São Luís", "lat": -2.5300, "lon": -44.3030},     "MG": {"cidade": "Belo Horizonte", "lat": -19.9227, "lon": -43.9450},     "MS": {"cidade": "Campo Grande", "lat": -20.4427, "lon": -54.6468},     "MT": {"cidade": "Cuiabá", "lat": -15.5989, "lon": -56.1088},     "PA": {"cidade": "Belém", "lat": -1.4558, "lon": -48.5044},     "PB": {"cidade": "João Pessoa", "lat": -7.1185, "lon": -34.8740},     "PE": {"cidade": "Recife", "lat": -8.0578, "lon": -34.8829},     "PI": {"cidade": "Teresina", "lat": -5.0917, "lon": -42.8039},     "PR": {"cidade": "Curitiba", "lat": -25.4290, "lon": -49.2665},     "RJ": {"cidade": "Rio de Janeiro", "lat": -22.9068, "lon": -43.1729},     "RN": {"cidade": "Natal", "lat": -5.7945, "lon": -35.2101},     "RO": {"cidade": "Porto Velho", "lat": -8.7608, "lon": -63.8967},     "RR": {"cidade": "Boa Vista", "lat": 2.8182, "lon": -60.6714},     "RS": {"cidade": "Porto Alegre", "lat": -30.0346, "lon": -51.2177},     "SC": {"cidade": "Florianópolis", "lat": -27.5935, "lon": -48.5585},     "SE": {"cidade": "Aracaju", "lat": -10.9472, "lon": -37.0731},     "SP": {"cidade": "São Paulo", "lat": -23.5505, "lon": -46.6333}, "TO": {"cidade": "Palmas", "lat": -10.2484, "lon": -48.3269} }
    RESULTADOS = {
    "dados_cidades": []
}
 # Realiza a chamada da API para a cidade em questão usando coordenadas geográficas.

    
    for capital in MAPA_COORDENADAS:
        CIDADE_NOME = MAPA_COORDENADAS[capital]["cidade"]
        LATITUDE = MAPA_COORDENADAS[capital]["lat"]
        LONGITUDE = MAPA_COORDENADAS[capital]["lon"]
        
        params = {
            'lat': LATITUDE,
            'lon': LONGITUDE,
            'appid': API_KEY,
        }
 # Consulta do clima para o cálculo de probabilidade de chuva. A probabilidade de chuva é calculada a partir de dois dados: umidade e  nuvens.      
        try:
            print(f"Consultando clima para: {CIDADE_NOME} ({LATITUDE}, {LONGITUDE})")
            
            response = requests.get(URL_BASE, params=params)
            response.raise_for_status()
            
            dados = response.json()
            
            if dados.get("cod") == 200:
                
                umidade = dados["main"]["humidity"]
                nuvens = dados["clouds"]["all"]
                probabilidade_de_chuva = ((0.6*(nuvens/100))+(0.4*(umidade/100)))*100
                
               
                print("✔️ Consulta Bem-Sucedida!")
                print(f"Probabilidade de Chuva: {round(probabilidade_de_chuva, 2)}%")
                print("-" * 30)
                RESULTADOS["dados_cidades"].append({CIDADE_NOME: round(probabilidade_de_chuva, 2)})
                                
            else:
                print(f"❌ Erro na API: Código {dados.get('cod')}. Mensagem: {dados.get('message')}")
                
        except requests.exceptions.RequestException as e:
            print(f"❌ Erro de Conexão ou HTTP: {e}")

        time.sleep(1)

# Criar um arquivo JSON com os dados obtidos.
    datenow = datetime.now().isoformat()
    RESULTADOS["publicacao"] = datenow
    
    with open("cidades.json", "w", encoding="utf-8") as arquivo_json:
        json.dump(RESULTADOS, arquivo_json, indent=4, ensure_ascii=False)

# --- EXECUÇÃO ---
if __name__ == "__main__":
    buscar_clima_capital()

