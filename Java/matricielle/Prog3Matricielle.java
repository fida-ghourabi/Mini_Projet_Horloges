import java.io.*;
import java.net.*;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

public class Prog3Matricielle {
    static final int MON_ID = 3;
    static final int PORT1 = 6001, PORT2 = 6002, PORT3 = 6003, PORT4 = 6004;
    static final int NB_PROC = 4;
    static final String IP = "127.0.0.1";
    static final AtomicBoolean stop = new AtomicBoolean(false);

    static int[][] matrixClock = new int[NB_PROC][NB_PROC];

    static void displayClock(String event) {
        System.out.printf("[P%d] %s | Horloge matricielle :\n", MON_ID, event);
        for (int[] row : matrixClock) {
            System.out.println(Arrays.toString(row));
        }
    }

    static void updateOnReceive(MessageMatriciel msg) {
        System.out.printf("[P%d] Recu de P%d | Matrice recue :\n", MON_ID, msg.senderId);
        for (int[] row : msg.matrix) System.out.println(Arrays.toString(row));

        for (int i = 0; i < NB_PROC; i++) {
            for (int j = 0; j < NB_PROC; j++) {
                matrixClock[i][j] = Math.max(matrixClock[i][j], msg.matrix[i][j]);
            }
        }
        matrixClock[MON_ID - 1][MON_ID - 1]++;
        displayClock("Mise a jour apres reception");
    }

    static void sendMessage(int port, int destId) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(IP, port), 2500);
            ObjectOutputStream out = new ObjectOutputStream(socket.getOutputStream());

            matrixClock[MON_ID - 1][MON_ID - 1]++;
            MessageMatriciel msg = new MessageMatriciel(MON_ID, matrixClock);
            System.out.printf("[P%d] Envoi a P%d | Matrice envoyee :\n", MON_ID, destId);
            for (int[] row : matrixClock) System.out.println(Arrays.toString(row));
            out.writeObject(msg);
        } catch (IOException e) {
            System.out.printf("[P%d] Connexion echouee au port %d (%s)%n", MON_ID, port, e.getMessage());
        }
    }

    static class Receiver extends Thread {
        public void run() {
            try (ServerSocket serverSocket = new ServerSocket(PORT3)) {
                System.out.printf("[P%d] Serveur pret sur le port %d%n", MON_ID, PORT3);
                while (!stop.get()) {
                    try (Socket clientSocket = serverSocket.accept();
                         ObjectInputStream in = new ObjectInputStream(clientSocket.getInputStream())) {
                        MessageMatriciel msg = (MessageMatriciel) in.readObject();
                        updateOnReceive(msg);
                    } catch (Exception e) {
                        System.out.printf("[P%d] Erreur reception : %s%n", MON_ID, e.getMessage());
                    }
                }
            } catch (IOException e) {
                System.out.printf("[P%d] Erreur serveur : %s%n", MON_ID, e.getMessage());
            }
        }
    }

    public static void main(String[] args) throws Exception {
        new Receiver().start();
        Thread.sleep(1000);

        matrixClock[MON_ID - 1][MON_ID - 1]++; displayClock("Evenement local 1 : Affichage");
        matrixClock[MON_ID - 1][MON_ID - 1]++; int x = 10; x++; displayClock("Evenement local 2 : Incrementation");
        matrixClock[MON_ID - 1][MON_ID - 1]++; System.out.printf("[P%d] Heure systeme : %s\n", MON_ID, new java.util.Date()); displayClock("Evenement local 3 : Heure systeme");
        matrixClock[MON_ID - 1][MON_ID - 1]++; int y = x / 2; displayClock("Evenement local 4 : Division");
        Thread.sleep(1000); matrixClock[MON_ID - 1][MON_ID - 1]++; displayClock("Evenement local 5 : Pause 1s");

        sendMessage(PORT1, 1); Thread.sleep(200);
        sendMessage(PORT2, 2); Thread.sleep(200);
        sendMessage(PORT4, 4); Thread.sleep(200);
        sendMessage(PORT1, 1);

        Thread.sleep(5000);
        stop.set(true);
        System.out.println("Appuyez sur Entrée pour quitter...");
        System.in.read();
    }
}