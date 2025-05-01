import java.io.Serializable;

public class Message implements Serializable {
    int senderId;
    int scalarClock;

    public Message(int senderId, int scalarClock) {
        this.senderId = senderId;
        this.scalarClock = scalarClock;
    }
}
