import java.io.Serializable;

public class Message implements Serializable {
    public int senderId;
    public int scalarClock;

    public Message(int senderId, int scalarClock) {
        this.senderId = senderId;
        this.scalarClock = scalarClock;
    }
}
