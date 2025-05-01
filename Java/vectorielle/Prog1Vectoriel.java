
import java.io.*;
import java.net.*;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

public class Prog1Vectoriel {
    static final int MON_ID = 1;
    static final int PORT1 = 6001, PORT2 = 6002, PORT3 = 6003, PORT4 = 6004;
    static final int NB_PROC = 4;
    static final String IP = "127.0.0.1";
    static final AtomicBoolean stop = new AtomicBoolean(false);

    static int[] vectorClock = new int[NB_PROC];

    static void displayClock(String event) {
        System.out.printf("[P%d] %s | Horloge vectorielle : %s%n", MON_ID, event, Arrays.toString(vectorClock));
    }

    static void updateOnReceive(MessageVectoriel msg) {
        System.out.printf("[P%d] Recu de P%d | Vecteur recu : %s%n", MON_ID, msg.senderId, Arrays.toString(msg.vector));
        for (int i = 0; i < NB_PROC; i++) {
            vectorClock[i] = Math.max(vectorClock[i], msg.vector[i]);
        }
        vectorClock[MON_ID - 1]++;
        System.out.printf("[P%d] Nouvelle horloge : %s%n", MON_ID, Arrays.toString(vectorClock));
    }

    static void sendMessage(int port, int destId) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(IP, port), 2500);
            ObjectOutputStream out = new ObjectOutputStream(socket.getOutputStream());

            vectorClock[MON_ID - 1]++;
            MessageVectoriel msg = new MessageVectoriel(MON_ID, vectorClock);
            System.out.printf("[P%d] Envoi a P%d | Horloge envoyee : %s%n", MON_ID, destId, Arrays.toString(vectorClock));
            out.writeObject(msg);
        } catch (IOException e) {
            System.out.printf("[P%d] Connexion echouee au port %d (%s)%n", MON_ID, port, e.getMessage());
        }
    }

    static class Receiver extends Thread {
        public void run() {
            try (ServerSocket serverSocket = new ServerSocket(PORT1)) {
                System.out.printf("[P%d] Serveur pret sur le port %d%n", MON_ID, PORT1);
                while (!stop.get()) {
                    try (Socket clientSocket = serverSocket.accept();
                         ObjectInputStream in = new ObjectInputStream(clientSocket.getInputStream())) {
                        MessageVectoriel msg = (MessageVectoriel) in.readObject();
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

        vectorClock[MON_ID - 1]++; displayClock("Evenement local 1 : Affichage");
        vectorClock[MON_ID - 1]++; int x = 4; x++; displayClock("Evenement local 2 : Incrementation");
        vectorClock[MON_ID - 1]++; System.out.printf("[P%d] Heure systeme : %s", MON_ID, new java.util.Date()); displayClock("Evenement local 3 : Heure systeme");
        vectorClock[MON_ID - 1]++; int y = x + 5; displayClock("Evenement local 4 : Addition");
         Thread.sleep(1000); vectorClock[MON_ID - 1]++; displayClock("Evenement local 5 : Pause 1s");

        sendMessage(PORT2, 2); Thread.sleep(200);
        sendMessage(PORT3, 3); Thread.sleep(200);
        sendMessage(PORT4, 4); Thread.sleep(200);
        sendMessage(PORT2, 2);

        Thread.sleep(5000);
        stop.set(true);
        System.out.println("Appuyez sur Entrée pour quitter...");
        System.in.read();
    }
}
