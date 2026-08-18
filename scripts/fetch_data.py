#!/usr/bin/env python3
"""
Script d'extraction et de formatage de données financières pour le Quant Engine C++.
Utilisation : python scripts/fetch_data.py --symbols AAPL,MSFT,GOOGL,MC.PA --start 2016-01-01 --end 2026-01-01
"""

import argparse
import csv
import os
from pathlib import Path
import yfinance as yf
import pandas as pd


def fetch_and_format_data(symbol: str, start_date: str, end_date: str, output_dir: Path) -> Path:
    print(f"[+] Téléchargement des données pour {symbol} du {start_date} au {end_date}...")
    
    # 1. Utilisation de yf.Ticker avec auto_adjust=True pour retrouver les vrais prix ajustés
    ticker = yf.Ticker(symbol)
    df = ticker.history(start=start_date, end=end_date, interval="1d", auto_adjust=True)
    
    if df.empty:
        print(f"[-] AVERTISSEMENT : Aucune donnée retournée pour le symbole '{symbol}'. Ignoré.")
        return None

    os.makedirs(output_dir, exist_ok=True)
    file_path = output_dir / f"{symbol}.csv"

    # 2. Écriture directe via la bibliothèque standard CSV
    rows = []
    for dt, row in df.iterrows():
        ts = int(dt.timestamp())
        
        rows.append({
            'timestamp': ts,
            'open': f"{row['Open']:.2f}",
            'high': f"{row['High']:.2f}",
            'low': f"{row['Low']:.2f}",
            'close': f"{row['Close']:.2f}",
            'volume': int(row['Volume'])
        })

    # 3. Écriture dans le fichier CSV
    fieldnames = ['timestamp', 'open', 'high', 'low', 'close', 'volume']
    with open(file_path, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"[✓] Données enregistrées avec succès dans : {file_path}")
    print(f"    - Nombre de barres enregistrées : {len(rows)}")
    print(f"    - Aperçu des 3 premières lignes :")
    for r in rows[:3]:
        print(f"  {r['timestamp']} {r['open']} {r['high']} {r['low']} {r['close']} {r['volume']}")
    print("-" * 60)

    return file_path


def main():
    parser = argparse.ArgumentParser(description="Fetch and format market data for C++ Quant Engine.")
    # On accepte --symbols (ou --symbol pour rétrocompatibilité)
    parser.add_argument("--symbols", "--symbol", type=str, default="AAPL", 
                        help="Ticker(s) à télécharger séparés par des virgules (ex: AAPL,MSFT,GOOGL)")
    parser.add_argument("--start", type=str, default="2016-01-01", help="Date de début (YYYY-MM-DD)")
    parser.add_argument("--end", type=str, default="2026-01-01", help="Date de fin (YYYY-MM-DD)")
    
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    data_dir = project_root / "data"

    # Découpage et nettoyage de la liste de symboles
    symbol_list = [s.strip().upper() for s in args.symbols.split(",") if s.strip()]

    print(f"[*] Traitement de {len(symbol_list)} symbole(s) : {', '.join(symbol_list)}\n")

    for symbol in symbol_list:
        try:
            fetch_and_format_data(symbol, args.start, args.end, data_dir)
        except Exception as e:
            print(f"[-] Erreur lors du traitement de {symbol} : {e}")


if __name__ == "__main__":
    main()