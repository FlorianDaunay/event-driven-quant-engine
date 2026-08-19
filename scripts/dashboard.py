import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import dash
from dash import dcc, html, Input, Output

# --- 1. Chargement et Préparation des Données ---
def load_data(filepath="trades.csv"):
    try:
        df = pd.read_csv(filepath)
        df["open_date"] = pd.to_datetime(df["open_date"])
        df["roll_date"] = pd.to_datetime(df["roll_date"])
        df = df.sort_values("roll_date").reset_index(drop=True)
        
        # Prise en compte du cashflow net des options cumulé
        df["cumulative_option_cashflow"] = df["net_cashflow"].cumsum()
        
        return df
    except Exception as e:
        print(f"Erreur lors du chargement du fichier CSV : {e}")
        return pd.DataFrame()

df_trades = load_data()

# --- 2. Initialisation de l'App Dash ---
app = dash.Dash(__name__, title="Quant Engine - Portfolio Dashboard")

app.layout = html.Div(style={
    'backgroundColor': '#121212',
    'color': '#ffffff',
    'fontFamily': 'Segoe UI, Tahoma, Geneva, Verdana, sans-serif',
    'padding': '20px'
}, children=[
    
    # En-tête
    html.H1("Global Strategy & Portfolio Dashboard", style={'textAlign': 'center', 'marginBottom': '25px', 'color': '#00d2d3'}),

    # Filtre par Ticker
    html.Div([
        html.Label("Actif / Stratégie :", style={'fontSize': '16px', 'marginRight': '10px'}),
        dcc.Dropdown(
            id='ticker-filter',
            options=[{'label': 'Tous les actifs', 'value': 'ALL'}] + 
                    [{'label': f"Focus {t}", 'value': t} for t in sorted(df_trades['ticker'].unique())] if not df_trades.empty else [],
            value='ALL',
            clearable=False,
            style={'width': '220px', 'color': '#000000'}
        )
    ], style={'marginBottom': '25px', 'display': 'flex', 'alignItems': 'center'}),

    # Métriques Clés Globale (KPIs)
    html.Div(id='kpi-container', style={'display': 'flex', 'justifyContent': 'space-between', 'marginBottom': '25px'}),

    # Graphiques Subplots Synchronisés
    dcc.Graph(id='main-dashboard-graph')
])

# Utilitaire de génération de carte KPI
def create_kpi_card(title, value, color, subtitle=None, is_global=False):
    base_style = {
        'backgroundColor': '#252830' if is_global else '#1e1e1e',
        'borderRadius': '8px',
        'padding': '15px 18px',
        'width': '15%',
        'boxShadow': '0 4px 12px rgba(0,0,0,0.6)' if is_global else '0 4px 10px rgba(0,0,0,0.4)',
        'borderTop': f'4px solid {color}' if is_global else 'none',
        'borderLeft': f'5px solid {color}' if not is_global else 'none'
    }
    
    return html.Div(style=base_style, children=[
        html.Div(title, style={'fontSize': '11px', 'color': '#f1c40f' if is_global else '#aaa', 'fontWeight': 'bold' if is_global else 'normal', 'marginBottom': '5px'}),
        html.Div(value, style={'fontSize': '17px', 'fontWeight': 'bold', 'color': '#fff'}),
        html.Div(subtitle, style={'fontSize': '11px', 'color': '#888', 'marginTop': '4px'}) if subtitle else None
    ])

# --- 3. Callbacks dynamiques ---
@app.callback(
    [Output('kpi-container', 'children'),
     Output('main-dashboard-graph', 'figure')],
    [Input('ticker-filter', 'value')]
)
def update_dashboard(selected_ticker):
    if df_trades.empty:
        return [], go.Figure()

    # 1. Données globales immuables
    df_global_sorted = df_trades.sort_values("roll_date").copy()
    global_final_equity = df_global_sorted["portfolio_equity"].iloc[-1] if "portfolio_equity" in df_global_sorted else 0.0
    global_initial_capital = 10000000.0  # 10M$ Initial
    global_pnl = global_final_equity - global_initial_capital

    # 2. Données filtrées selon le menu déroulant
    filtered_df = df_trades if selected_ticker == 'ALL' else df_trades[df_trades['ticker'] == selected_ticker].copy()
    filtered_df = filtered_df.sort_values('roll_date')

    # 3. Calculs des flux options pour la sélection
    total_premium = filtered_df["premium_collected"].sum()
    total_payout = filtered_df["settlement_payout"].sum()
    net_option_cashflow = filtered_df["net_cashflow"].sum()

    # 4. Calcul dynamique du PnL selon la vue
    if selected_ticker == 'ALL':
        pnl_display = global_pnl
        ret_pct_display = (global_pnl / global_initial_capital) * 100.0
        stock_hedge_pnl = global_pnl - net_option_cashflow
        pnl_subtitle = f"Rendement Global: {ret_pct_display:+.2f}%"
    else:
        num_assets = len(df_trades["ticker"].unique())
        allocated_capital = global_initial_capital / max(1, num_assets)
        
        stock_hedge_pnl = (global_pnl - df_trades["net_cashflow"].sum()) / max(1, num_assets)
        pnl_display = stock_hedge_pnl + net_option_cashflow
        ret_pct_display = (pnl_display / allocated_capital) * 100.0
        pnl_subtitle = f"Contribution {selected_ticker}: {ret_pct_display:+.2f}%"

    # 5. Cartes KPI
    kpis = [
        create_kpi_card("Valeur Portefeuille (Global)", f"${global_final_equity:,.2f}", "#f1c40f", "Fixe - Tous Actifs", is_global=True),
        create_kpi_card(f"PnL Stratégie ({selected_ticker})", f"${pnl_display:,.2f}", "#10ac84" if pnl_display >= 0 else "#ee5253", pnl_subtitle),
        create_kpi_card("PnL Delta Hedge (Actions)", f"+${stock_hedge_pnl:,.2f}", "#54a0ff", "Gains sur sous-jacent"),
        create_kpi_card("Net Cashflow Options", f"${net_option_cashflow:,.2f}", "#ee5253" if net_option_cashflow < 0 else "#10ac84", "Primes - Payouts"),
        create_kpi_card("Primes Encaissées", f"+${total_premium:,.2f}", "#10ac84", "Total ventes Calls"),
        create_kpi_card("Payouts Expiration", f"${total_payout:,.2f}", "#ee5253", "Règlements ITM")
    ]

    # --- Construction de la Figure Subplots (4 Cadrants) ---
    fig = make_subplots(
        rows=2, cols=2,
        shared_xaxes=False,
        vertical_spacing=0.12,
        horizontal_spacing=0.08,
        subplot_titles=(
            "Évolution Globale de la Valeur du Portefeuille (Equity $)",
            "Détail du Cashflow Options Cumulé ($)",
            "Ventilation par Trade : Primes vs Payouts",
            "Moyenne des Primes Encaissées par Ticker ($)"
        )
    )

    # 1. Courbe d'Equity Réelle du Portefeuille (Global Fixe)
    fig.add_trace(
        go.Scatter(
            x=df_global_sorted["roll_date"],
            y=df_global_sorted["portfolio_equity"],
            mode="lines",
            name="Global Portfolio Equity",
            line=dict(
                color="#f1c40f", 
                width=2.5, 
                shape="spline",       # Lisse les variations brutales
                smoothing=0.8         # Degré de lissage (entre 0 et 1.3)
            ),
            hovertemplate="<b>Date</b>: %{x|%Y-%m-%d}<br><b>Portfolio Equity</b>: $%{y:,.2f}<extra></extra>"
        ),
        row=1, col=1
    )

    # Force le premier quadrant à ne pas démarrer l'axe Y à zéro (autoscale ajusté)
    fig.update_yaxes(zeroline=False, row=1, col=1)

    # 2. Cashflow Négatif/Positif Séparé de la Jambe Options (Dynamique)
    fig.add_trace(
        go.Scatter(
            x=filtered_df["roll_date"],
            y=filtered_df["cumulative_option_cashflow"],
            mode="lines",
            name="Cashflow Options Cumulé",
            line=dict(color="#ff9f43", width=2, dash="dash"),
            hovertemplate="<b>Date</b>: %{x|%Y-%m-%d}<br><b>Cashflow Options</b>: $%{y:,.2f}<extra></extra>"
        ),
        row=1, col=2
    )

    # 3. Stacked Bar Chart: Primes Encaissées vs Payouts Expiration (Dynamique)
    fig.add_trace(
        go.Bar(
            x=filtered_df["roll_date"],
            y=filtered_df["premium_collected"],
            name="Primes (+)",
            marker_color="#10ac84",
            hovertemplate="<b>Ticker</b>: " + filtered_df["ticker"] + "<br><b>Prime Encaissée</b>: +$%{y:,.2f}<extra></extra>"
        ),
        row=2, col=1
    )
    fig.add_trace(
        go.Bar(
            x=filtered_df["roll_date"],
            y=filtered_df["settlement_payout"],
            name="Payouts (-)",
            marker_color="#ee5253",
            hovertemplate="<b>Payout Expiration</b>: $%{y:,.2f}<extra></extra>"
        ),
        row=2, col=1
    )

    # 4. Bar Chart Comparatif par Ticker (Primes Moyennes)
    ticker_summary = df_trades.groupby("ticker")["premium_collected"].mean().reset_index()
    fig.add_trace(
        go.Bar(
            x=ticker_summary["ticker"],
            y=ticker_summary["premium_collected"],
            name="Prime Moyenne",
            marker_color="#54a0ff",
            hovertemplate="<b>Ticker</b>: %{x}<br><b>Prime Moyenne</b>: $%{y:,.2f}<extra></extra>"
        ),
        row=2, col=2
    )

    # Layout Dark Theme
    fig.update_layout(
        template="plotly_dark",
        height=750,
        barmode="relative",
        showlegend=False,
        paper_bgcolor="#121212",
        plot_bgcolor="#1e1e1e",
        margin=dict(l=40, r=40, t=60, b=40)
    )

    return kpis, fig

# --- 4. Exécution du Serveur ---
if __name__ == '__main__':
    app.run_server(debug=True, port=8050)