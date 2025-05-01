import java.io.*;
import java.net.*;
import java.util.concurrent.atomic.AtomicBoolean;

public class Prog3Scalar {
    static final int MON_ID = 3;
    static final int PORT1 = 6001, PORT2 = 6002, PORT3 = 6003, PORT4 = 6004;
    static int scalarClock = 0;
    static final String IP = "127.0.0.1";
    static AtomicBoolean stop = new AtomicBoolean(false);

    static void displayClock(String event) {
        System.out.printf("[P%d] %s | Horloge scalaire : %d%n", MON_ID, event, scalarClock);
    }

    static void updateOnReceive(Message msg) {
        scalarClock = Math.max(scalarClock, msg.scalarClock) + 1;
        System.out.printf("[P%d] Recu de P%d | Horloge recue : %d -> Nouvelle horloge : %d%n",
                MON_ID, msg.senderId, msg.scalarClock, scalarClock);
    }

    static void sendMessage(int port, int destId) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(IP, port), 2500);
            ObjectOutputStream out = new ObjectOutputStream(socket.getOutputStream());

            // Incrémentation de l'horloge avant l'envoi
            scalarClock++;
            Message msg = new Message(MON_ID, scalarClock);

            System.out.printf("[P%d] Envoi a P%d | Horloge scalaire envoyee : %d%n", MON_ID, destId, scalarClock);
            out.writeObject(msg);

        } catch (IOException e) {
            System.out.printf("[P%d] Connexion echouee au port %d (%s)%n", MON_ID, port, e.getMessage());
        }
    }

    static class Receiver extends Thread {
        public void run() {
            try (ServerSocket serverSocket = new ServerSocket(PORT3)) {
                System.out.printf("[P%d] Serveur en ecoute sur le port %d%n", MON_ID, PORT3);
                while (!stop.get()) {
                    try (Socket clientSocket = serverSocket.accept();
                         ObjectInputStream in = new ObjectInputStream(clientSocket.getInputStream())) {
                        Message msg = (Message) in.readObject();
                        updateOnReceive(msg);
                    } catch (Exception e) {
                        System.out.println("[P" + MON_ID + "] Erreur lors de la réception du message: " + e.getMessage());
                    }
                }
            } catch (IOException e) {
                System.out.println("[P" + MON_ID + "] Erreur serveur : " + e.getMessage());
            }
        }
    }

    public static void main(String[] args) throws Exception {
        new Receiver().start();
        Thread.sleep(1000);

        // Evénement local 1
        scalarClock++;
        displayClock("Evenement local 1 : Affichage");

        // Evénement local 2
        scalarClock++;
        int x = 7;
        x--;
        displayClock("Evenement local 2 : Decrementation");

        // Evénement local 3
        scalarClock++;
        System.out.printf("[P%d] Heure systeme : %s", MON_ID, new java.util.Date());
        displayClock("Evenement local 3 : Heure systeme");

        // Evénement local 4
        scalarClock++;
        int y = x / 3;
        displayClock("Evenement local 4 : Division");

        // Evénement local 5
        scalarClock++;
        displayClock("Evenement local 5 : Pause 1s");

        // Envoi des messages après l'incrémentation et l'affichage
        sendMessage(PORT1, 1);
        Thread.sleep(200);
        sendMessage(PORT2, 2);
        Thread.sleep(200);
        sendMessage(PORT4, 4);
        Thread.sleep(200);
        sendMessage(PORT1, 1);

        Thread.sleep(5000);
        stop.set(true);
        System.out.println("Appuyez sur Entrée pour quitter...");
        System.in.read();
    }
}
