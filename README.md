# Lab 23 — Intégration JNI & Protection Anti-Debug Native sous Android

Ce projet présente le développement d'une application Android complète nommée **JNIDemo** écrite en **Java** et intégrant des modules natifs en **C++** via l'interface **JNI (Java Native Interface)** et le système de construction **CMake**.

L'objectif principal de ce laboratoire est de déplacer des contrôles de sécurité critiques (anti-analyse, détection de rétro-ingénierie et d'instrumentation dynamique) au niveau de la couche native C++, rendant ainsi les vérifications beaucoup plus difficiles à contourner ou à neutraliser que lors de simples implémentations côté Java.

---

## 🎯 Objectifs pédagogiques

- **Maîtriser l'environnement de développement natif (NDK & JNI)** : Comprendre comment lier des méthodes Java à du code C++ natif.
- **Mettre en œuvre des techniques défensives anti-débogage** : Utiliser les appels système bas niveau pour détecter la surveillance de processus.
- **Analyser la cartographie mémoire à bas niveau** : Inspecter en direct le fichier virtuel de processus `/proc/self/maps` pour y repérer des outils d'instrumentation.
- **Séparer les responsabilités (Natif / Java)** : Remonter les indicateurs de compromission vers la couche applicative managée, en laissant l'interface utilisateur s'adapter élégamment au lieu de simplement crasher l'application.

---

## 🗺️ Architecture de l'Application

Le projet possède la structure native hybride suivante :

```
com.example.jnidemo
└── MainActivity.java           # Gère le cycle de vie applicatif, l'interface graphique et la réaction post-détection

app/src/main/cpp
├── CMakeLists.txt              # Script de configuration CMake pour la compilation et le linkage natif
└── native-lib.cpp              # Code C++ contenant les algorithmes de détection ptrace et d'analyse mémoire
```

---

## 🛠️ Description des Algorithmes Défensifs Natifs

### 1. Détection de traçage via l'appel système `ptrace`
L'appel système Linux `ptrace` permet à un processus parent d'observer et de contrôler l'exécution d'un autre processus (ce qui est typiquement le cas lors du débogage d'une application native via `gdb` ou `lldb`).
Sous Linux, un processus ne peut être tracé (`PTRACE_TRACEME`) que par un seul superviseur à la fois. La fonction native tente donc d'appeler :
```cpp
ptrace(PTRACE_TRACEME, 0, 0, 0);
```
Si le résultat renvoyé est `-1`, cela signifie qu'un processus de traçage (débogueur) s'est déjà attaché à l'application.

### 2. Inspection dynamique de `/proc/self/maps`
Le noyau Linux maintient pour chaque processus un fichier virtuel `/proc/self/maps` qui liste toutes les bibliothèques logicielles partagées (`.so`) chargées en mémoire ainsi que les plages d'adresses allouées.
L'application ouvre ce fichier à bas niveau et effectue une recherche textuelle sélective (via `strstr`) pour détecter des chaînes associées à des outils d'analyse bien connus :
- **Frida** (`frida`, `libfrida`) : Framework d'instrumentation dynamique réputé.
- **Xposed** (`xposed`) : Framework de hooking Android.
- **Magisk** (`magisk`) : Outil de rootage.
- **GDB/LLDBSERVER** (`gdbserver`, `libgdb`) : Serveurs de débogage à distance.

---

## 💻 Code Source Principal

### C++ : `native-lib.cpp`
```cpp
#include <jni.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <android/log.h>
#include <sys/ptrace.h>
#include <unistd.h>

#define LOG_TAG "ANTI_DEBUG"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static bool isBeingTraced() {
    long result = ptrace(PTRACE_TRACEME, 0, 0, 0);
    return (result == -1);
}

static bool containsSuspiciousLibraryNames() {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return false;
    char line[512];
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "frida") || strstr(line, "xposed") ||
            strstr(line, "libfrida") || strstr(line, "gdbserver") ||
            strstr(line, "libgdb") || strstr(line, "magisk")) {
            fclose(maps);
            return true;
        }
    }
    fclose(maps);
    return false;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_jnidemo_MainActivity_isDebugDetected(JNIEnv* env, jobject) {
    return (isBeingTraced() || containsSuspiciousLibraryNames()) ? JNI_TRUE : JNI_FALSE;
}
```

---

## 🧪 Scénarios de Validation du Laboratoire

1.  **Exécution en environnement standard (Sécurité : OK)** :
    *   Lancez l'application normalement sur un appareil physique non modifié.
    *   L'interface affiche un statut vert "**Etat securite : OK**" et débloque le calcul natif factoriel.

2.  **Exécution sous Débogueur Actif (Détection Active)** :
    *   Lancez l'application en mode débogage depuis Android Studio (qui y attache automatiquement `lldb`).
    *   Le module `ptrace` intercepte l'appel système et renvoie l'indicateur d'environnement suspect.
    *   L'activité Java capte l'indicateur, bascule l'affichage au **ROUGE**, affiche le message "**environnement suspect detecte**", et bloque l'accès aux fonctions natives sensibles de l'application.