from pymongo import MongoClient
from config import MONGO_URI, MONGO_DATABASE, USE_MOCK_DB
from unittest.mock import MagicMock 
import sys
import os

COLLECTION_NAME = "measurements"

def get_mongo_collection():
    # MODO MOCK 
    if USE_MOCK_DB:
        print(f"Devolviendo colección simulada de MongoDB: {COLLECTION_NAME}")
        mock_collection = MagicMock()
        mock_collection.insert_one.side_effect = lambda doc: print(f"Documento guardado simuladamente: {doc.get('serial', 'N/A')}")
        return mock_collection

    # MODO REAL
    try:
        client = MongoClient(MONGO_URI)
        mongo_db = client[MONGO_DATABASE]
        return mongo_db[COLLECTION_NAME]
    except Exception as e:
        print(f"Falló la conexión a MongoDB: {e}", file=sys.stderr)
        os._exit(1)