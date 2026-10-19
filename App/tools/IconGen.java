import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.geom.Ellipse2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.image.BufferedImage;
import java.io.File;
import javax.imageio.ImageIO;

/**
 * Draws the app icon (a little robot car with OLED eyes and the distance sensor on top)
 * in every size Android needs. Run:  java IconGen.java <the app's res folder>
 */
public class IconGen {
    public static void main(String[] args) throws Exception {
        String res = args[0];
        String[] folders = {"mipmap-mdpi", "mipmap-hdpi", "mipmap-xhdpi", "mipmap-xxhdpi", "mipmap-xxxhdpi"};
        int[] sizes = {48, 72, 96, 144, 192};
        for (int i = 0; i < sizes.length; i++) {
            File dir = new File(res, folders[i]);
            dir.mkdirs();
            ImageIO.write(draw(sizes[i]), "png", new File(dir, "ic_launcher.png"));
        }
        ImageIO.write(draw(512), "png", new File(res, "../../icon-512.png"));
    }

    static BufferedImage draw(int size) {
        BufferedImage img = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = img.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        double s = size;

        // background: dark rounded square
        g.setColor(new Color(0x1B2838));
        g.fill(new RoundRectangle2D.Double(0, 0, s, s, s * 0.44, s * 0.44));

        // the distance sensor on top: a little bar with two silver "eyes"
        g.setColor(new Color(0x1565C0));
        g.fill(new RoundRectangle2D.Double(s * 0.30, s * 0.17, s * 0.40, s * 0.15, s * 0.05, s * 0.05));
        g.setColor(new Color(0xCFD8DC));
        g.fill(new Ellipse2D.Double(s * 0.335, s * 0.185, s * 0.12, s * 0.12));
        g.fill(new Ellipse2D.Double(s * 0.545, s * 0.185, s * 0.12, s * 0.12));
        g.setColor(new Color(0x607D8B));
        g.fill(new Ellipse2D.Double(s * 0.365, s * 0.215, s * 0.06, s * 0.06));
        g.fill(new Ellipse2D.Double(s * 0.575, s * 0.215, s * 0.06, s * 0.06));

        // wheels
        g.setColor(new Color(0x546E7A));
        g.fill(new Ellipse2D.Double(s * 0.10, s * 0.66, s * 0.24, s * 0.24));
        g.fill(new Ellipse2D.Double(s * 0.66, s * 0.66, s * 0.24, s * 0.24));
        g.setColor(new Color(0xECEFF1));
        g.fill(new Ellipse2D.Double(s * 0.18, s * 0.74, s * 0.08, s * 0.08));
        g.fill(new Ellipse2D.Double(s * 0.74, s * 0.74, s * 0.08, s * 0.08));

        // orange body
        g.setColor(new Color(0xFF8F00));
        g.fill(new RoundRectangle2D.Double(s * 0.08, s * 0.32, s * 0.84, s * 0.42, s * 0.20, s * 0.20));

        // the OLED screen with two eyes
        g.setColor(new Color(0x0B0F14));
        g.fill(new RoundRectangle2D.Double(s * 0.22, s * 0.38, s * 0.56, s * 0.29, s * 0.08, s * 0.08));
        g.setColor(Color.WHITE);
        g.fill(new RoundRectangle2D.Double(s * 0.30, s * 0.43, s * 0.15, s * 0.19, s * 0.07, s * 0.07));
        g.fill(new RoundRectangle2D.Double(s * 0.55, s * 0.43, s * 0.15, s * 0.19, s * 0.07, s * 0.07));

        // a happy little smile under the eyes
        g.setColor(new Color(0xFFE082));
        g.setStroke(new BasicStroke((float) (s * 0.025), BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
        g.drawArc((int) (s * 0.43), (int) (s * 0.585), (int) (s * 0.14), (int) (s * 0.06), 200, 140);

        g.dispose();
        return img;
    }
}
