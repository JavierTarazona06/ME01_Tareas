import numpy as np
import matplotlib.pyplot as plt
from typing import List, Optional


def graph_line(
        x: np.ndarray, ys: List[np.ndarray],
        labelys: Optional[List[str]] = None,
        title: Optional[str] = "Línea continua",
        labelx: Optional[str] = "x", labely: Optional[str] = "y",
        store_path: Optional[str] = "grafica.png",
        colors: Optional[List[str]] = None,
        linestyles: Optional[List[str]] = None,
        marker: Optional[str] = 'o',
        figsize: Optional[tuple] = (10, 6),
        dpi: Optional[int] = 150
) -> None:
    """
    Grafica múltiples series de datos en un mismo gráfico
    """
    # Configuración por defecto
    labelys = labelys or [f"Serie {i + 1}" for i in range(len(ys))]
    colors = colors or plt.cm.tab10.colors[:len(ys)]
    linestyles = linestyles or ['-'] * len(ys)

    # Crear figura
    plt.figure(figsize=figsize, dpi=dpi)

    # Graficar cada serie
    for i, (y, label, color, linestyle) in enumerate(zip(ys, labelys, colors, linestyles)):
        plt.plot(
            x, y,
            linestyle=linestyle,
            marker=marker,
            color=color,
            linewidth=2,
            markersize=6,
            label=label
        )

    # Personalización del gráfico
    plt.xlabel(labelx, fontsize=12)
    plt.ylabel(labely, fontsize=12)
    plt.title(title, fontsize=14, pad=20)
    plt.grid(True, linestyle='--', alpha=0.5)

    # Añadir leyenda si hay múltiples series
    if len(ys) > 1:
        plt.legend(fontsize=10, framealpha=1)

    # Ajustar márgenes y guardar
    plt.tight_layout()
    plt.savefig(store_path, bbox_inches='tight', dpi=dpi)
    plt.close()