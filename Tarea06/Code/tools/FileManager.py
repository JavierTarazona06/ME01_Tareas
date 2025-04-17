import json
from typing import Any, Dict

class JSON_man:
    @staticmethod
    def dict2json(data_dict: Dict[str, Any], path: str) -> None:
        """
        Guarda un diccionario como archivo JSON en una ruta especificada.
        """
        print(f"Guardando archivo JSON en: {path}")
        with open(path, 'w', encoding='utf-8') as file:
            json.dump(data_dict, file, indent=2, ensure_ascii=False)

    @staticmethod
    def json2dict(path: str) -> Dict[str, Any]:
        """
        Carga un archivo .json desde la ruta especificada y
        lo convierte en un diccionario.
        """
        print(f"Cargando archivo JSON desde: {path}")
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data