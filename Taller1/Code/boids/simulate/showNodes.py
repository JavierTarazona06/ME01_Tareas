import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

# Leer datos
df = pd.read_csv("boids_positions.csv")

# Configurar figura
fig, ax = plt.subplots(figsize=(10, 8))
ax.set_xlim(df["X"].min() - 10, df["X"].max() + 10)
ax.set_ylim(df["Y"].min() - 10, df["Y"].max() + 10)
ax.set_title("Simulación Boids con Líderes")

# Colores (rojo=leader, azul=follower)
colors = np.where(df["IsLeader"] == 1, "red", "blue")


# Animación
def update(frame):
    ax.clear()
    current_data = df[df["Time"] == frame]
    ax.scatter(
        current_data["X"],
        current_data["Y"],
        c=current_data["IsLeader"].map({1: "red", 0: "blue"}),
        s=50,
        alpha=0.7,
    )
    ax.set_title(f"Time: {frame:.2f}s")

    # Dibujar áreas de influencia para líderes
    leaders = current_data[current_data["IsLeader"] == 1]
    for _, leader in leaders.iterrows():
        circle = plt.Circle((leader["X"], leader["Y"]), 100, color="red", alpha=0.1)
        ax.add_patch(circle)

    ax.set_xlim(df["X"].min() - 10, df["X"].max() + 10)
    ax.set_ylim(df["Y"].min() - 10, df["Y"].max() + 10)


# Crear animación
frames = sorted(df["Time"].unique())
ani = FuncAnimation(fig, update, frames=frames, interval=100)

plt.show()
# Para guardar: ani.save('boids_simulation.mp4', writer='ffmpeg', fps=10)
