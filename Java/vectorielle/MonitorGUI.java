import javax.swing.*;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashMap;

public class MonitorGUI extends JFrame {
    private final HashMap<Integer, JTextArea> processAreas = new HashMap<>();

    public MonitorGUI() {
        setTitle("🕒 Monitor - Horloge Vectorielle");
        setSize(1200, 800);  // Plus grand pour permettre une meilleure vue
        setLayout(new GridLayout(2, 2, 20, 20));  // 2 lignes et 2 colonnes pour un affichage moderne
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        // Ajouter de l'espace autour des éléments (margin padding)
        Insets padding = new Insets(15, 15, 15, 15);

        // Créer une zone de texte pour chaque processus (P1 à P4)
        for (int i = 1; i <= 4; i++) {
            JTextArea area = new JTextArea(15, 40);  // Grande zone de texte
            area.setEditable(false);
            area.setFont(new Font("SansSerif", Font.PLAIN, 16));  // Font plus grande pour meilleure lisibilité
            area.setBackground(new Color(240, 240, 240));  // Fond gris clair
            area.setMargin(padding);  // Marge pour le texte

            JScrollPane scrollPane = new JScrollPane(area);
            scrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);

            // Bordure et titre du processus avec plus de style
            scrollPane.setBorder(BorderFactory.createTitledBorder(
                    BorderFactory.createLineBorder(new Color(33, 150, 243), 3), // Bordure bleue
                    "Processus P" + i,
                    TitledBorder.CENTER,
                    TitledBorder.TOP,
                    new Font("SansSerif", Font.BOLD, 18),
                    new Color(33, 150, 243)  // Couleur du texte dans le titre
            ));

            processAreas.put(i, area);
            add(scrollPane);
        }

        setVisible(true);

        // Démarrer le serveur de réception des messages des processus
        new Thread(() -> {
            try (ServerSocket server = new ServerSocket(7002)) {
                System.out.println("🎧 MonitorGUI en écoute sur le port 7002...");
                while (true) {
                    try (Socket client = server.accept();
                         BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()))) {

                        String line = in.readLine();
                        if (line != null && line.startsWith("[P")) {
                            int pid = Integer.parseInt(line.substring(2, 3));
                            logEvent(pid, line);
                        }

                    } catch (Exception e) {
                        System.out.println("Erreur réception GUI : " + e.getMessage());
                    }
                }
            } catch (Exception e) {
                System.out.println("Impossible de démarrer le serveur GUI : " + e.getMessage());
            }
        }).start();
    }

    // Méthode pour ajouter un message dans l'interface pour un processus donné
    public void logEvent(int processId, String message) {
        SwingUtilities.invokeLater(() -> {
            JTextArea area = processAreas.get(processId);
            if (area != null) {
                area.append(message + "\n");
                area.setCaretPosition(area.getDocument().getLength()); // Scroll automatique
            }
        });
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(MonitorGUI::new);
    }
}
