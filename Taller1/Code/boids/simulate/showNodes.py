import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
from matplotlib.widgets import CheckButtons

# Leer datos
df = pd.read_csv("boids_positions.csv")

# Configurar figura
fig, ax = plt.subplots(figsize=(12, 8))
plt.subplots_adjust(left=0.1, right=0.9, top=0.9, bottom=0.2)  # Espacio para controles

# Establecer límites iniciales
ax.set_xlim(df["X"].min() - 10, df["X"].max() + 10)
ax.set_ylim(df["Y"].min() - 10, df["Y"].max() + 10)
ax.set_title("Simulación Boids con Líderes y Fuegos")

# Variables para controlar la visualización
show_fires = True
show_leader_zones = True

# Crear controles de visualización
ax_check = plt.axes([0.1, 0.05, 0.2, 0.1])
check = CheckButtons(ax_check, ['Mostrar Fuegos', 'Mostrar Zonas Líder'], [True, True])

def toggle_visibility(label):
    global show_fires, show_leader_zones
    if label == 'Mostrar Fuegos':
        show_fires = not show_fires
    elif label == 'Mostrar Zonas Líder':
        show_leader_zones = not show_leader_zones
    # No necesitamos redibujar aquí, se hará en el próximo frame

check.on_clicked(toggle_visibility)

# Animación
def update(frame):
    ax.clear()
    current_data = df[df["Time"] == frame]
    
    # Separar boids y fuegos
    boids = current_data[current_data["IsFire"] == 0]
    fires = current_data[current_data["IsFire"] == 1]
    
    # Dibujar boids (líderes y seguidores)
    ax.scatter(
        boids["X"],
        boids["Y"],
        c=boids["IsLeader"].map({1: "red", 0: "blue"}),
        s=50,
        alpha=0.7,
    )
    
    # Dibujar fuegos si está activado
    if show_fires and not fires.empty:
        ax.scatter(
            fires["X"],
            fires["Y"],
            c="yellow",
            marker="^",
            s=100,
            alpha=1.0,
            label="Fuegos"
        )
    
    # Dibujar áreas de influencia si está activado
    if show_leader_zones:
        leaders = boids[boids["IsLeader"] == 1]
        for _, leader in leaders.iterrows():
            circle = plt.Circle((leader["X"], leader["Y"]), 
                             100, 
                             color="red", 
                             alpha=0.1)
            ax.add_patch(circle)
    
    ax.set_title(f"Time: {frame:.2f}s")
    ax.set_xlim(df["X"].min() - 10, df["X"].max() + 10)
    ax.set_ylim(df["Y"].min() - 10, df["Y"].max() + 10)
    
    # Mostrar leyenda
    handles, labels = ax.get_legend_handles_labels()
    if handles:  # Solo mostrar leyenda si hay elementos
        ax.legend(handles, labels, loc='upper right')

# Crear animación
frames = sorted(df["Time"].unique())
ani = FuncAnimation(fig, update, frames=frames, interval=100)  # Limitar a 100 frames para prueba

plt.show()